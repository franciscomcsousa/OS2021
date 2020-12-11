# SO-Project

## Visão global do projeto

O objetivo final do projeto é desenvolver um sistema de ficheiros (File System, FS) em modo utilizador
(user-level) e que mantém os seus conteúdos em memória primária, chamado TecnicoFS.

# Arquitetura

Para suportar acessos concorrentes e otimizar o desempenho, as funcionalidades do TecnicoFS são oferecidas, 
em paralelo, por um conjunto de tarefas escravas no processo servidor do FS.

A API do TecnicoFS oferecerá as seguintes funções básicas:
● Criar, pesquisar, remover e renomear ficheiro/diretoria
● Abrir, ler, escrever e fechar ficheiro 

# Estado do TecnicoFS

Tal como em FS tradicionais modernos, o conteúdo do FS encontra-se referenciado numa estrutura
de dados principal chamada tabela de i-nodes, global ao FS. Cada i-node representa uma diretoria ou
um ficheiro no TecnicoFS, que tem um identificador único chamado i-number. O i-number de uma
diretoria/ficheiro corresponde ao índice do i-node correspondente na tabela de i-nodes. O i-node
consiste numa estrutura de dados que descreve os atributos da diretoria/ficheiro (aquilo que
normalmente se chamam os seus meta-dados) e que referencia o conteúdo da diretoria/ficheiro (ou
seja, os dados).

Além da tabela de i-nodes, existe uma região de dados. Esta região mantém os dados de todos os
ficheiros do FS, sendo esses dados referenciados a partir do i-node de cada ficheiro (na tabela de inodes). No caso de ficheiros normais, é na região de dados que é mantido o conteúdo do ficheiro (por
exemplo, a sequência de caracteres que compõe um ficheiro de texto). No caso de diretorias, a região
de dados mantém a respetiva tabela, que representa o conjunto de ficheiros (ficheiros normais e subdiretorias) que existem nessa diretoria.

Cada entrada na tabela de uma diretoria consiste num par (nome, i-number). O conjunto destes pares
permite enumerar quais os nomes dos ficheiros incluídos numa diretoria, assim como aceder aos
dados e meta-dados desses ficheiros (indexando a tabela de i-nodes pelo i-number do ficheiro
pretendido). 
