# Algoritmo de Banker

Implementação em C do algoritmo de Banker para prevenção de deadlocks, proposta na atividade de Infraestrutura de Software. O programa lê a matriz de máximo de cada cliente e uma sequência de comandos de requisição, liberação e impressão de estado, gravando o resultado em `result.txt`.

## Requisitos

- Compilador C compatível com C11 (`gcc`)
- `make`
- Ambiente Linux, Unix ou macOS

## Compilação

```sh
make
```

O comando gera o executável `banker` no diretório atual. Para remover o executável e os arquivos gerados pela compilação:

```sh
make clean
```

## Execução

```sh
./banker 10 5 7
```

Cada argumento representa a quantidade de instâncias de um tipo de recurso. A quantidade de argumentos define o número de tipos de recursos do sistema. No exemplo, existem três tipos de recursos com 10, 5 e 7 instâncias, respectivamente.

## Arquivos de entrada

Os arquivos `customer.txt` e `commands.txt` devem estar no mesmo diretório em que o programa é executado. Eles não fazem parte do repositório e são fornecidos separadamente.

### customer.txt

Cada linha representa um cliente e contém os valores da matriz `MAXIMUM`, separados por vírgula. A quantidade de linhas define o número de clientes, e cada linha deve ter exatamente a mesma quantidade de valores que o número de tipos de recursos informado na linha de comando.

Exemplo para três tipos de recursos e cinco clientes:

```text
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

### commands.txt

Cada linha contém um comando. Os valores são separados por espaços ou tabulações.

- `RQ cliente r1 r2 ...`: requisição de recursos feita pelo cliente
- `RL cliente r1 r2 ...`: liberação de recursos feita pelo cliente
- `*`: imprime o estado atual do sistema

Exemplo:

```text
RQ 0 0 2 1
RQ 2 3 0 2
RQ 1 2 0 0
RQ 4 0 0 2
RL 0 0 1 1
RQ 3 2 1 1
*
RQ 4 4 0 0
RQ 1 1 0 2
RQ 0 0 2 0
RQ 2 0 1 0
```

## Arquivo de saída

O programa cria o arquivo `result.txt` no diretório de execução. Para cada comando, uma mensagem é gravada conforme o resultado da operação:

- alocação concedida;
- requisição negada por exceder a necessidade máxima do cliente;
- requisição negada por falta de recursos disponíveis;
- requisição negada porque o novo estado seria inseguro;
- liberação concedida;
- liberação negada por exceder a alocação atual do cliente.

O comando `*` imprime as colunas `MAXIMUM`, `ALLOCATION` e `NEED` de todos os clientes, além do vetor `AVAILABLE`.

## Tratamento de erros

Antes de executar os comandos, o programa valida os argumentos e o conteúdo dos arquivos de entrada. Uma mensagem é exibida em `stderr` e a execução é encerrada com código de erro quando:

- nenhum argumento é informado;
- algum argumento não é um inteiro positivo;
- a soma das instâncias dos recursos ultrapassa o limite de um inteiro;
- `customer.txt` ou `commands.txt` não existe ou não pode ser aberto;
- alguma linha possui formato inválido ou quantidade de valores diferente do número de tipos de recursos;
- os arquivos de entrada estão vazios;
- o estado inicial informado pela matriz `MAXIMUM` é inseguro;
- `result.txt` não pode ser criado ou gravado.

Quando a execução termina com sucesso, o código de retorno é `0`. Em qualquer erro de validação ou de execução, o código de retorno é `1`.

## Exemplo de uso

Com os arquivos de exemplo apresentados acima:

```sh
make
./banker 10 5 7
cat result.txt
```

A saída contém as alocações e liberações aceitas, a impressão do estado solicitada por `*` e as mensagens das requisições negadas, na mesma ordem em que os comandos aparecem em `commands.txt`.

## Evidências de execução

O arquivo `evidencias.log` registra a compilação, a execução com as entradas de exemplo, os testes com entradas inválidas e a verificação de vazamentos de memória com Valgrind.
