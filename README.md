# Projeto de PCA

## Descricao

Trabalho final de PCA, nesse trabalho foram comparados 3 protocolos de PSI, Bitwise-And, Pairwise Comparisons e um protocolo proposto pelo artigo usado como referência SCS-SORT.

## Como executar

O projeto usa C++17 e precisa do compilador `g++` instalado.

### Linux

No terminal, entre na pasta do projeto e execute o script:

```bash
./executar.sh
```

Caso o script nao tenha permissao de execucao, use:

```bash
chmod +x executar.sh
./executar.sh
```

### Windows

No Prompt de Comando ou PowerShell, entre na pasta do projeto e execute o arquivo `.bat`:

```bat
executar.bat
```

O programa ira pedir qual arquivo CSV deve ser usado e salvara o resultado em `saida.txt`.

## Observacoes

Para entradas são usados os arquivos csv dentro da pasta data, 3 arquivos de teste foram criados, um favorável a cada algoritmo, caso queira criar mais arquivos de testes siga o padrão dos testes já feitos.

## Autores

João Francisco Gomes Targino
Kássio Medeiros Alves 
