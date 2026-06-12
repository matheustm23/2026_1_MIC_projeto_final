# Pastas das equipes

Cada equipe deve criar sua pasta seguindo o padrão `equipe-XX/`, onde `XX` é o
número da equipe (01, 02, 03, ...), contendo o projeto **completo** do Microchip
Studio. Não fragmente o projeto em subpastas por aluno ou por desafio - a
organização interna do código é responsabilidade da equipe.

A pasta é criada na primeira contribuição da equipe, com um novo projeto **GCC C
Executable Project** (device **ATmega328P**) no Microchip Studio. Os drivers
(UART, ADC, PWM, etc.) devem ser implementados pela própria equipe - use como
referência o portfólio da disciplina:
[github.com/adankvitschal/portfolio_mic](https://github.com/adankvitschal/portfolio_mic).

Estrutura esperada:

```text
equipes/
  equipe-01/
    equipe-01.cproj   (ou nome equivalente gerado pelo Microchip Studio)
    main.c
    ...
  equipe-02/
  ...
```
