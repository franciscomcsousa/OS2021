# Exercicio 3

## 1. Comunicação com processos cliente
O servidor TecnicoFS deixa de carregar comandos a partir de um ficheiro. Em vez disso, passa a ter um
socket Unix do tipo datagram, através do qual recebe (e responde a) pedidos de operações solicitadas
por outros processos, que designamos de processos cliente.

O executável do TecnicoFS passa a receber os seguintes argumentos de linha de comandos:
`tecnicofs numthreads nomesocket`
O novo argumento, nomesocket, indica o nome que deve ser associado ao socket pelo qual o servidor
receberá pedidos de ligação. 

Pretende-se desenvolver dois componentes principais: o servidor TecnicoFS e a biblioteca cliente. De
seguida descrevemos cada um em detalhe.

1) O servidor TecnicoFS deve ter em conta que:
- A tarefa inicial é responsável por inicializar o socket.
- Cada tarefa escrava recebe pedidos enviados através do socket, em vez de os receber a partir
do vetor de comandos. Esta mudança torna o vetor de comandos obsoleto, pelo que este deve
ser removido do código do servidor TecnicoFS.
- Após receber e executar um pedido de operação, a tarefa escrava deve devolver o resultado
da operação numa mensagem enviada pelo mesmo socket, endereçada ao socket do cliente
que originou o pedido.
- Por simplicidade do projeto, deve assumir-se que o servidor nunca termina.

2) Quanto à biblioteca cliente:
- A biblioteca chama-se tecnicofs-client-api, cujo esqueleto é fornecido no código
base disponibilizado no site da disciplina.
- Em teoria, a biblioteca pode ser usada por diferentes programas cliente.
- A biblioteca implementa o protocolo de comunicação o cliente e o servidor TecnicoFS.
Essencialmente, a biblioteca fornece uma função para cada operação do servidor TecnicoFS.
Quando chamada, cada função envia um pedido ao servidor TecnicoFS e aguarda pela sua
resposta.
Para auxiliar o desenvolvimento e os testes do 3º exercício, é também fornecido um programa cliente
de exemplo. Este cliente é sequencial (single-threaded). Essencialmente, este cliente carrega uma
sequência de comandos a partir de um ficheiro de entrada e, um por um, invoca-os sobre o servidor
TecnicoFS.
Quanto ao protocolo que define as mensagens trocadas entre cliente e servidor:
- Os pedidos enviados pelos clientes consistem em strings indicando a operação solicitada e
o(s) seus(s) argumentos(s), sendo o formato dessas strings aquele que foi definido nos
enunciados anteriores para cada operação do TecnicoFS.
- A resposta consiste no inteiro que é devolvido pela operação invocada (o valor do inteiro a
enviar ao cliente deve ser o mesmo que já era devolvido nos projetos anteriores). 

## 2. Nova operação ‘p’

O TecnicoFS deve passar a suportar uma nova operação que imprime o seu conteúdo atual (através
da função já existente print_tecnicofs_tree) para um ficheiro de saída indicado como único argumento
da operação
