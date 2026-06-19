/*
Alterações data 19/06/2026
PROJETO FINAL DA UC DE MICROCONTROLADORES:
CONTROLE DE TEMPERATURA COM LM35

Alunos: Luis Iope e Matheus Machado

N�cleo obrigat�rio:
Leitura do LM35 via ADC e convers�o para �C
Sa�da PWM controlando a pot�ncia da l�mpada
UART TX (envio da temperatura)
UART RX (recebimento de comandos)
Controle ON-OFF

Desafios extra:
Setpoint via bot�es f�sicos (feito)
@Luis adicionar aqui os desafios que conseguirmos implementar

*/

#define F_CPU 16000000
#include <xc.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/eeprom.h> //Biblioteca pra leitura e escrita na EEPROM interna

//Vari�veis globais:
uint16_t gTemperatura = 0; //Temperatura atual em �C
uint16_t gSetpoint = 25; //Temperatura desejada em �C
uint8_t gLampadaLigada = 0; //Flag para l�mpada (0 para desligada, 1 para ligada)
uint16_t gLimiarAlarme = 25; //Temperatura que dispara o alarme em °C (editável via serial: AL=)
uint8_t gAlarmeAtivo = 0; //Flag que indica se o alarme já foi enviado (evita ficar mandando toda hora)

uint8_t *gEepromIndice = (uint8_t*)0; //Endereço onde fica salvo o índice atual do buffer circular
uint8_t *gEepromHistorico = (uint8_t*)1; //Endereço inicial do vetor de histórico (ocupa 10 bytes a partir daqui)
uint8_t gHistIndice = 0; //Índice atual do buffer circular (cópia em RAM do que está na EEPROM)
uint16_t gUltimaTempGravada = 0xFFFF; //Guarda a última temperatura gravada, pra só escrever na EEPROM quando o valor mudar

#define UART_BUFFER_SIZE 16 //Texto recebido pela uart de no m�x 16 caracteres
#define HISTORICO_TAMANHO 10 //Quantidade de amostras guardadas no histórico circular da EEPROM

//Vari�veis pra comunica��o serial:
volatile char gUartBuffer[UART_BUFFER_SIZE]; //string que guarda os caracteres que chegam
volatile uint8_t gUartIndex = 0; //guarda posi��o atual onde o pr�ximo caractere ser� salvo no vetor
volatile uint8_t gComandoPronto = 0; //flag que vira 1 quando o usu�rio aperta enter no terminal

//Protótipos das funções (declaradas aqui em cima pra poderem ser chamadas em qualquer ordem no arquivo)
void uart_init(uint32_t tBaud);
void uart_putchar(char tDado);
void uart_print(const char *tStr);
void processar_comando(void);
void salvar_historico_eeprom(void);
void enviar_historico_eeprom(void);

ISR(USART_RX_vect) //Interrup��o solicitada toda vez que um caractere chega no pino RX
{
	char tByte = UDR0; //L� o registrador onde o caractere recebido fica guardado e salva na vari�vel tempor�ria tByte

	if (tByte == '\n' || tByte == '\r') //Verifica se o caractere recebido foi uma quebra de linha (\n) ou um Enter (\r)
	{
		if (gUartIndex > 0) //Garante que o usu�rio digitou alguma coisa antes de apertar Enter (�ndice tem que ser maior que zero)
		{
			gUartBuffer[gUartIndex] = '\0'; //Adiciona o caractere nulo no final do buffer transformando o vetor em uma string v�lida no C
			gComandoPronto = 1; //Sinaliza para o programa principal que h� um comando completo esperando para ser processado
			gUartIndex = 0; //Reseta o �ndice para que o pr�ximo comando comece a ser gravado do in�cio do vetor
		}
	}
	/*Se o caractere n�o for um enter ele entra aqui, verifica se ainda h� espa�o no 
	buffer para evitar estouro de mem�ria (UART_BUFFER_SIZE - 1),se houver espa�o, 
	o caractere � salvo no buffer e o �ndice � incrementado (gUartIndex++)*/
	else if (gUartIndex < (UART_BUFFER_SIZE - 1))
	{
		gUartBuffer[gUartIndex++] = tByte;
	}
}

void uart_init(uint32_t tBaud)//Fun��o que configura a velocidade e os pinos da comunica��o serial
{
	uint16_t tUbrr = (F_CPU / (16UL * tBaud)) - 1; //Equa��o da tabela 19-1 do datasheet pro c�lculo da taxa de transmiss�o (UBRR)

	UBRR0H = (uint8_t)(tUbrr >> 8);
	UBRR0L = (uint8_t)tUbrr;

	UCSR0B = (1<<TXEN0) //Habilita transmiss�o
		   | (1<<RXEN0) //Habilita recep��o
		   | (1<<RXCIE0); //Habilita interrup��o de recep��o
		   
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); //frame de 8 bits, sem paridade e 1 bit de parada
}

/*
Fun��o que envia um �nico caractere, o while fica travado esperando o bit UDRE0 
(do registrador UCSR0A) ficar em 1, o que significa que o hardware terminou de 
enviar o caractere anterior e o buffer de transmiss�o est� vazio, quando libera, 
ele joga o caractere em UDR0 para ser transmitido fisicamente
*/
void uart_putchar(char tDado)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = tDado;
}

/*
Fun��o que recebe um ponteiro para um texto (string) e vai enviando caractere 
por caractere usando a fun��o uart_putchar at� encontrar o fim do texto (\0).
*/
void uart_print(const char *tStr)
{
	while (*tStr)
	uart_putchar(*tStr++);
}

/*
Cria uma c�pia local (tComando) do buffer global da UART. Isso serve para liberar o 
buffer original de forma segura ou manipul�-lo sem interfer�ncias.
*/
void processar_comando(void)
{
	char tComando[UART_BUFFER_SIZE];
	
	//Copia o conte�do do buffer global para uma vari�vel local, evita que um novo dado chegue pela UART enquanto outro dado esteja sendo processado
	for (uint8_t i = 0; i < UART_BUFFER_SIZE; i++)
	tComando[i] = gUartBuffer[i];
	
	//Verifica se o texto enviado come�a exatamente com as letras "SP=" (Set Point).
	if (tComando[0]=='S' && tComando[1]=='P' && tComando[2]=='=')
	{
		uint16_t tNovoSetpoint = (uint16_t)atoi(&tComando[3]);

		if (tNovoSetpoint <= 110)
		{
			gSetpoint = tNovoSetpoint;
			uart_print("OK\r\n");
		}
		else
		{
			uart_print("ERRO: setpoint fora da faixa (0-110)\r\n");
		}
	}
	
	//Verifica se o texto enviado começa exatamente com as letras "AL=" (Alarme).
	else if (tComando[0]=='A' && tComando[1]=='L' && tComando[2]=='=')
	{
		uint16_t tNovoLimiar = (uint16_t)atoi(&tComando[3]);

		if (tNovoLimiar <= 110)
		{
			gLimiarAlarme = tNovoLimiar;
			uart_print("OK\r\n");
		}
		else
		{
			uart_print("ERRO: limiar fora da faixa (0-110)\r\n");
		}
	}
	
	//Verifica se o texto enviado é exatamente o comando "HIST" (Histórico).
	else if (tComando[0]=='H' && tComando[1]=='I' && tComando[2]=='S' && tComando[3]=='T')
	{
		enviar_historico_eeprom();
	}
	else
	{
		uart_print("ERRO: comando invalido\r\n");
	}
}

/*
Função que salva a temperatura atual na EEPROM em formato de buffer circular.
Só grava se a temperatura mudou desde a última gravação, pra preservar a vida
útil da EEPROM (limite de ~100mil ciclos de escrita por byte). Quando o índice
chega no fim do vetor, ele volta pro início (sobrescrevendo a leitura mais antiga).
*/
void salvar_historico_eeprom(void)
{
	if (gTemperatura != gUltimaTempGravada)
	{
		eeprom_update_byte(gEepromHistorico + gHistIndice, (uint8_t)gTemperatura);

		gHistIndice++;
		if (gHistIndice >= HISTORICO_TAMANHO)
		gHistIndice = 0;

		eeprom_update_byte(gEepromIndice, gHistIndice); //Salva o índice atualizado pra sobreviver a um reset

		gUltimaTempGravada = gTemperatura;
	}
}

/*
Função que lê o histórico salvo na EEPROM e manda pela serial, da amostra mais
antiga pra mais recente, na ordem correta (começando pela posição seguinte ao
índice atual, já que ali está a gravação mais antiga do buffer circular).
*/
void enviar_historico_eeprom(void)
{
	char tMsg[16];
	uart_print("HISTORICO:\r\n");

	for (uint8_t i = 0; i < HISTORICO_TAMANHO; i++)
	{
		uint8_t tPos = (gHistIndice + i) % HISTORICO_TAMANHO;
		uint8_t tValor = eeprom_read_byte(gEepromHistorico + tPos);

		sprintf(tMsg, "%u: %u C\r\n", i + 1, tValor);
		uart_print(tMsg);
	}
}

int main(void)
{
	DDRC &= ~((1<<DDC0) | (1<<DDC1)); //PC0 e PC1 como entrada
	PORTC |= (1<<PORTC0) | (1<<PORTC1); //Ativa pull-up do PC0 e PC1

	DDRD |= (1<<DDD5); //PD5 como sa�da (PWM que controla a potencia da lampada)
	DDRD |= (1<<DDD7); //PD7 como saída (aciona o buzzer do alarme sonoro)

	//Modo fast PWM
	TCCR0A = (1<<COM0B1) | (1<<WGM01) | (1<<WGM00);
	TCCR0B = (1<<WGM02) | (1<<CS01);

	OCR0A = 99;
	OCR0B = 0;

	ADMUX = (1<<REFS1)|(1<<REFS0) //Referencia de tens�o interna de 1,1V
	| (0<<MUX3)|(1<<MUX2)|(0<<MUX1)|(1<<MUX0); //ADC5

	ADCSRA = (1<<ADEN) //Habilita ADC
	| (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Prescaler do ADC em 128

	DIDR0 = (1<<ADC5D); //Desabilita buffer do ADC5 que j� est� sendo usado como entrada anal�gica

	uart_init(9600); //inicializa uart com 9600 de baud

	sei(); //habilita interrup��es globais
	
	gHistIndice = eeprom_read_byte(gEepromIndice); //Recupera o índice do histórico salvo na EEPROM (sobrevive a reset/desligamento)
	if (gHistIndice >= HISTORICO_TAMANHO) //Proteção: se a EEPROM nunca foi gravada (vem com 0xFF de fábrica), zera o índice
	gHistIndice = 0;

	while (1)
	{
		ADCSRA |= (1<<ADSC);
		while (ADCSRA & (1<<ADSC));
		
		//Converte valor bruto do ADC (0-1023) em �C
		gTemperatura = ((uint32_t)ADC * 1100) / 1024 / 10;
		
		//Salva a leitura atual no histórico da EEPROM (buffer circular de 10 posições)
		salvar_historico_eeprom();

		//Controle ON-OFF com histerese de +- 1�C
		if (!gLampadaLigada && gTemperatura <= (gSetpoint - 1))
		{
			gLampadaLigada = 1;
			OCR0B = 99;
		}

		if (gLampadaLigada && gTemperatura >= (gSetpoint + 1))
		{
			gLampadaLigada = 0;
			OCR0B = 0;
		}
		
		//Se o bot�o for pressionado decrementa SP
		if (!(PINC & (1<<PINC0)))
		{
			if (gSetpoint > 0)
			gSetpoint--;

			while (!(PINC & (1<<PINC0)));
			_delay_ms(100);
		}
		
		//Se o bot�o for pressionado aumenta SP
		if (!(PINC & (1<<PINC1)))
		{
			if (gSetpoint < 110)
			gSetpoint++;

			while (!(PINC & (1<<PINC1)));
			_delay_ms(100);
		}
		
		//Verifica se a temperatura passou do limiar de alarme (definido via serial com AL=) e dispara o aviso pela serial
		if (gTemperatura > gLimiarAlarme && !gAlarmeAtivo)
		{
			gAlarmeAtivo = 1;
			PORTD |= (1<<PORTD7); //Liga o buzzer
			uart_print("ALARME: TEMP_ALTA\r\n");
		}

		if (gTemperatura <= gLimiarAlarme && gAlarmeAtivo)
		{
			gAlarmeAtivo = 0;
			PORTD &= ~(1<<PORTD7); //Desliga o buzzer
			uart_print("ALARME: TEMP_NORMALIZADA\r\n");
		}

		//String com temperatura atual e setpoint enviada pela serial
		char tMsg[32];
		sprintf(tMsg, "TEMP=%u;SET=%u\r\n", gTemperatura, gSetpoint);
		uart_print(tMsg);

		//Se um comando chegar pela UART processa esse comando e zera para n�o ser processado novamente
		if (gComandoPronto)
		{
			gComandoPronto = 0;
			processar_comando();
		}

		_delay_ms(250);
	}
}
