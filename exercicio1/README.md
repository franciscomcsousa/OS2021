# Exercicio 1

Pretende-se desenvolver uma primeira versão paralela do servidor do TecnicoFS. Esta versão ainda
não suportará a interação com os processos clientes (isso só será suportado no 3º exercício). Em
alternativa, deve executar sequências de chamadas carregadas a partir de um ficheiro de entrada.
Para efeitos de depuração, quando o servidor terminar, este deve também apresentar o tempo total
de execução no stdout e escrever o conteúdo final da diretoria num ficheiro de saída.

O programa do servidor deve chamar-se tecnicofs e receber obrigatoriamente os seguintes quatro argumentos
de linha de comando.

`./tecnicofs inputfile outputfile numthreads synchstrategy`
