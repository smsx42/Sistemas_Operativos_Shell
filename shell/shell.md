# shell

### Búsqueda en $PATH

---
#### ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

execve recibe el nombre del programa, los argumentos y las variables de entorno necesarias para poder ejecutarlo. 

Los _wrappers_ ofrecen distinas maneras de hacer un _exec_ según convenga. Dadas esas tres partes veamos una por una. Primero, el programa a ejecutar puede ser provisto como un archivo o como un _path_; segundo, los argumentos pueden ser pasados como un vector (un array común y silvestre) o como una lista (los argumentos en sucesión separados por coma); tercero, las variables de entorno pueden ser modificadas mediante un vector o utilizar las mismas que están en la variable externa _environ_.

Los nombres de los _wrappers_ dan pistas sobre su uso mediante sufijos. 

Veamos:

Por la forma de pasar el programa (p):

- execlp, execvp, execvpe reciben un nombre de archivo.
- las que no incluyen "p" en el sufijo reciben un _path_ al programa.

Por la forma de pasar argumentos, vector (v) o lista (l):

- execv, execvp, execvpe reciben un vector.

- execl, execlp, execle reciben una lista.

Por la forma de pasar variables de entorno (e):

- execle, execvpe reciben un array de variables de entorno.

- execl, execlp, execv, execvp utilizan las variables de _environ_. 



#### ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

Sí, puede fallar. En caso de error _exec_ retorna -1 y se modifica la variable _errno_.

En nuestra shell se imprime el número de error y se vuelve a leer comandos.

### Procesos en segundo plano

---

#### Explicar detalladamente el mecanismo completo utilizado.

En un principio tenemos 2 variables compartidas entre archivos mediante extern, _bg_pgid_ y _bg_processes_count_, ambos inicializados como 0. bg_pgid representa el group id de los procesos en segundo plano y _bg_processes_count_ la cantidad de procesos en segundo plano, luego entraremos en detalle del uso de cada una.

Al recibir un comando de type BACK, se realiza un fork, donde el proceso hijo ejecuta el comando y el padre se encarga simplemente de imprimir el pid del proceso hijo y sumar 1 a _bg_processes_count_. No espera a que el hijo termine, permitiendo que la shell pueda seguir ejecutando comandos mientras el proceso hijo se ejecuta en segundo plano.

Si no hay ningún proceso en segundo plano (si el valor de _bg_pgid_ es 0), el proceso hijo setea su pgid igual que su pid, y el valor de _bg_pgid_ es actualizado a ese pid. En caso contrario, si había procesos en segundo plano, el proceso hijo setea su pgid como _bg_pgid_.

Los procesos hijos al terminar enviarán una señal SIGCHLD que será manejada por la función _sigchld_handler_. Esta función llama a waitpid en un ciclo while con el flag WNOHANG para que no sea bloqueante esta acción, con pid igual a _bg_pgid_ negativo: al recibir una señal SIGCHLD con pgid igual a _bg_pgid_, imprime que el proceso terminó y resta 1 a _bg_processes_count_. Si _bg_processes_count_ es 0, _bg_pgid_ se setea a 0, dado que al no haber más procesos con pgid igual a _bg_pgid_, este valor ya no es válido.

#### ¿Por qué es necesario el uso de señales?

El uso de señales en nuestro caso es necesario para poder trackear la finalización de los procesos en segundo plano ya que queremos liberar los recursos de ellos e imprimir que terminaron: al no esperar a que terminen éstos antes de recibir más comandos, necesitamos una forma de saber cuando terminen, y esa forma es mediante señales.

Los procesos pueden recibir señales de distintos tipos, y modificando el signal handler podemos decidir qué hacer con cada una de ellas. En este trabajo, utilizamos la señal SIGCHLD que envía el kernel al proceso padre cuando un hijo termina.

### Flujo estándar

### Investigar el significado de 2>&1, explicar cómo funciona su forma general
-2>&1 es una sintaxis utilizada para redirigir la salida de error estándar (stderr) a la salida estándar (stdout). 
Esta redirección es útil cuando se desea combinar ambas salidas en una sola

### Mostrar qué sucede con la salida de cat out.txt en el ejemplo.

-combina las salidas primero colaca el error y luego el stdout normal.

(/home/gm) 
$ ls -C /home /noexiste >out.txt 2>&1

(/home/gm) 
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
gm

### Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).

-Cambiar el orden no afecto la salida, en bash al cambiar el orden salio el error por pantalla y redirecciono la salida de ls /home a out.txt

$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
$ cat out.txt
/home:
gm

### Tuberías múltiples
### Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe
### ¿Cambia en algo?

-El exit code retorna al sistema operativo indicando si este se termino bien o con error 0 para una corrida satisfactoria y 1-255 para indicar el error.

### ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

- Caso proceso inicial falla : tanto bash como nuestra shell se comportan igual

 (/home/gm) 
$ cat non_existing_file.txt | grep "pattern" | wc -l
cat: non_existing_file.txt: No such file or directory
0

-Caso proceso intermedio fall:  ambos procesos fallan pero en el caso de bash paraece tener informacion extra sobre el fallo de grep.

bash:
$ echo -e "line1\npattern\nline3" | grep "[" | wc -l
grep: Invalid regular expression
0

nuestra shell:
 (/home/gm) 
$ echo -e "line1\npattern\nline3" | grep "[" | wc -l
grep: Unmatched [, [^, [:, [., or [=
0

-caso proceso en ultimo falla: Ambos procesos fallan en este caso nuestr shell informa del fallo en el exec pero en bash indica que el comando es invalido

bash:
$ ls /bin | grep "bash" | non_existent_command
non_existent_command: command not found

nuestra shell:
 (/home/gm) 
$ ls /bin | grep "bash" | non_existent_command
ERROR 2 AL hacer exec

### Variables de entorno temporarias

### ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Al momento de realizar un fork(2) se genera una copia exacta del proceso padre en el proceso hijo, provocando que ambos
procesos tengan un área de memoria separada, lo que genera que los cambios realizados en el proceso hijo no afecten al proceso padre. Estos cambios incluye el uso de variables de entorno temporales, en donde inicialmente el  hijo se inicializa con las mismas variables de entorno temporales que el padre, con la particularidad de que a medida que va avanzando la ejecución del programa, estas variables pueden agregarse o modificarse sin afectar al padre.

### En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).

### ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué. 

No van a tener el mismo comportamiento si se utiliza el tercer argumento de exec(3) o si se usa setenv(3). 
El tercer argumento de exec(3) me permite pasar una lista de nuevas variables de entorno que un dterminado proceso  puede utilizar. Supongamos que realizo un fork(2) antes de realizar un exec(3) (tengo entendido que es una práctica muy común), esto lo que va a generar es que antes del exec(3), tanto el proceso hijo como el padre tengan definidas las mismas variables de entorno; no obstante al momento de utilizar el tercer argumento del exec(3), el hijo utilizara las variables de entorno que se encuentren en la lista pasada y no heredara las variables del  padre. En cambio, utilizando setenv(3) se puede no solamente heredar las variables del padre, sino que además se pueden definir nuevas variables de entorno a medida que avanza la ejecución de un programa.

### Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Una posible implantación para tener un resultado similar seria definir una estructura que contenga las variables de entorno del padre y pasar dicha estructura como el tercer argumento al momento de realizar un exec(3). Esto permite que el hijo herede las variables de entorno del proceso original, y a su vez, pueda agregar otras variables sin modificar las originales. Otra forma de obtener el mismo resultado es que el tercer argumento sea Null, ya que como se dijo anteriormente, al momento de realizar fork(2) se terminan heredando las variables de entorno.

### Pseudo-variables

### Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

Existen varios tipos de variables mágicas que son utilizadas por la terminal. Se pueden encontrar:

- $- : Contiene las opciones que puede utilizar la shell actualmente. Por ejemplo, sin en la consola ejecuto echo $-, puede mostrar himBHS, en donde cada sigla representa una opcion diferente.

- $0 : Contiene el nombre del script que se está ejecutando actualmente; y en caso de que no se ejecuta ninguno, se mostrara el nombre del comando ejecutado en la actualidad. Por ejemplo, si en la terminal ejecuto $0,  se muestra por pantalla bash.

- $_ : Contiene el último argumento del script o comando ejecutado. En el contexto del primer ejemplo, si hago primero $- y luego echo $_, en la terminal se verá himBHS.


### Comandos built-in

---

#### ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

Como ya se ha aclarado _cd_ no puede ser buil-tin, _pwd_ en cambio, sí. 

Al hacerlo builtin se evita tener que hacer una _syscall_ por lo tanto,  teóricamente es más rápido llamar a un built-in que invocar a un programa externo.

### Historial

#### ¿Cuál es la función de los parámetros MIN y TIME del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a MIN en 1 y a TIME en 0?

Las funciones que cumplen los parámetros MIN y TIME en el modo no canónico son las siguientes:

- MIN: Indica el número mínimo de caracteres



### Aclaracion

Si bien todas las pruebas funcionan y la shell funciona correctamente, tenemos perdidas de memoria en el programa. Dichas perdidas no la pudimos solucionar, ya que al tratar de arreglarlas se rompían algunas pruebas; sin embargo tenemos una idea de en donde ocurren.
- En el archivo builtin.c en las diversas ocurre perdida en todas las funciones. Creemos que se debe a los strdup que si bien, sabemos que los tenemos que liberar, por algún motivo se rompen algunas pruebas al agregar los frees correspondientes para las variables copia_cmd.
- En el archivo parsing.c se nos detecta el error en la función parse_line debido a los mallocs de la variable commands. Nosotros creemos que la liberación debería realizarse en la función de run_cmd de run_cmd.c mediante la función free_commands, sin embargo, las pruebas rompen a pesar de que tratamos de utilizarla de distintas maneras.

