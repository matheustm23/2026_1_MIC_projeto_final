# Projeto Final - Microcontroladores (ATmega328P)

Repositório de entrega do projeto final da unidade curricular de **Microcontroladores**,
curso de Engenharia de Controle e Automação - IFSC Campus Chapecó.

## Objetivo

Implementar, no ATmega328P (placa Arduino, kit educacional Datapool), um sistema de
controle de temperatura em malha fechada:

- **Sensor**: LM35 (saída analógica), lido via ADC
- **Atuador**: lâmpada acionada por sinal PWM
- **Comunicação**: porta serial (UART) com o PC
  - o microcontrolador envia periodicamente a leitura de temperatura
  - o PC envia comandos para alterar o setpoint (e, quando aplicável, os termos
    Kp, Ki e Kd do PID)
- **Controle**: a estratégia (ON-OFF, PID, etc.) depende dos desafios escolhidos
  pela equipe - ver [docs/desafios.md](docs/desafios.md)

## Requisitos técnicos

- Desenvolvimento e compilação em **Microchip Studio** (Atmel Studio 7)
- Uso exclusivo das **bibliotecas Atmel/avr-libc** (`avr/io.h`, `avr/interrupt.h`,
  `util/delay.h`, etc.) - **não utilizar o framework Arduino** (`Arduino.h`, `Wire.h`,
  `Serial`, etc.)
- Microcontrolador **ATmega328P**, clock de **16 MHz** (padrão das placas Arduino
  Uno/Nano - ajuste `F_CPU` caso a placa da equipe seja diferente)
- Código gravado na placa Arduino do kit Datapool (via bootloader ou ISP)

## Material de apoio

Os drivers básicos (UART, ADC, PWM, etc.) não são fornecidos prontos: cada equipe
deve implementá-los a partir do datasheet do ATmega328P. Como referência, consulte
o portfólio desenvolvido ao longo do semestre:
[github.com/adankvitschal/portfolio_mic](https://github.com/adankvitschal/portfolio_mic).

## Estrutura do repositório

```text
README.md
docs/
  desafios.md          # lista de desafios extras e pontuação
equipes/
  equipe-01/           # projeto completo da equipe 1
  equipe-02/
  ...
```

Cada equipe trabalha em **uma única pasta** (`equipes/equipe-XX/`) contendo o projeto
completo do Microchip Studio. Não fragmente o projeto em subpastas por aluno ou por
desafio - a organização interna do código é responsabilidade da equipe.

## Como contribuir (fork + pull request)

Cada aluno deve enviar pelo menos uma contribuição através de um **pull request próprio**
(alunos sem PR mesclado não recebem a pontuação correspondente).

1. Faça um **fork** deste repositório na sua conta do GitHub.
2. Clone o seu fork:
   ```
   git clone https://github.com/SEU-USUARIO/NOME-DO-REPO.git
   ```
3. **Antes de começar**, sincronize seu fork com este repositório (botão
   "Sync fork" no GitHub) e atualize sua cópia local:
   ```
   git pull origin main
   ```
   Isso traz as contribuições já mescladas dos colegas de equipe.
4. Na primeira contribuição da equipe, crie a pasta `equipes/equipe-XX/`
   (substituindo `XX` pelo número da equipe) com um novo projeto **GCC C
   Executable Project** no Microchip Studio, device **ATmega328P**. Use como
   referência os drivers do portfólio da disciplina (ver "Material de apoio" acima).
5. Faça as alterações dentro de `equipes/equipe-XX/`, depois commit e push para o
   `main` do seu fork:
   ```
   git add .
   git commit -m "descrição da contribuição"
   git push origin main
   ```
6. Abra um **Pull Request** do seu fork (`main`) para o branch `main` deste
   repositório, descrevendo o que foi feito.

> Atenção: arquivos idênticos entregues por equipes diferentes serão identificados.
> Em caso de duplicidade, apenas o primeiro PR mesclado recebe a pontuação.
>
> Sincronize seu fork sempre que um colega de equipe tiver um PR mesclado, antes
> de iniciar uma nova contribuição - evita conflitos ao editar os mesmos arquivos.

## Avaliação

- **Funcionamento real** no hardware (kit Datapool)
- **Qualidade e organização do código** (avr-libc, sem dependência do framework Arduino)
- **Pontuação por desafios**: mínimo de 10 pontos, conforme [docs/desafios.md](docs/desafios.md)
- **Apresentação**: todos os integrantes da equipe devem estar presentes e ser
  capazes de explicar qualquer trecho do código
- **Pull requests individuais**: cada aluno deve ter ao menos um PR mesclado com
  contribuição própria identificável

## Próximos passos (planejado)

- Verificação automatizada de similaridade de código (além de arquivos idênticos)
- GitHub Actions para build automático dos projetos (compilação via avr-gcc)
