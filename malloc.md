# malloc

En este proyecto se desarrollará una librería de usuario que implementará las funciones malloc(3), calloc(3), realloc(3) y free(3).
La librería se encargará de solicitar la memoria que requiera,
y la administrará de forma transparente para el usuario.
Además, puede utilizarse de forma normal en cualquier programa de C.

### CONSTANTES 
___

MAX_LARGE_BLOCKS | MAX_LITTLE_BLOCKS | MAX_MID_BLOCKS: Màxima cantidad de bloques por
cada tipo del mismo (pequeños, medianos y grandes).

Cada tipo de bloque cuenta con un tamaño máximo:
 - Bloque pequeño: 16Kib
 - Bloque mediano: 1Mib
 - Bloque grande: 32Mib



### VARIABLES GLOBALES
___


Se utilizan 3 listas de bloques, una para cada tipo del mismo (little_blocks,  mid_blocks y large_blocks).
Ademas, también existe un puntero hacia el último bloque creado para un manejo más eficiente.

Se añadieron atributos al struct "malloc_stats" para poder crear los tests
del proyecto:
- amount_of_regions: Cantidad total de regiones (todas los bloques sumados).
- amount_little_blocks: Cantidad total de bloques pequeños.
- amount_of_mid_blocks: Cantidad total de bloques medianos.
- amount_of_large_blocks: Cantidad total de bloques grandes.

### STRUCTS
___


Definimos un nuevo tipo de struct el cual es "block", el mismo actúa como una lista
doblemente enlazada y tiene un puntero a su primera región.

Por otro lado, al struct de "region" le agregamos un puntero a la region anterior.

### BÚSQUEDA DE REGIONES
___


Definimos dos formas de busacar la region, una para cada tipo de busqueda (best fit y first fit).

Best fit utiliza la region libre mas chica dentro de todos los bloques que pueda alocar el tamaño solicitado.

First fit utiliza la primera region libre de memoria que cumpla que pueda alocar el tamaño solicitado.

En ambos casos, los algoritmos se realizan a nivel de tipo de bloque, primero se empieza con la lista de bloques de tamaño más
chico que puede alocar la memoria pedida. Si no encuentra ninguno, se sigue con la siguiente lista.

Si los algoritmos no encuentran regiones libre, la función devolverá NULL y se deberá crear un nuevo bloque.


### COALESCING
___

El feature de coalescing se utiliza para la agrupación de regiones libres y contiguas dentro de un bloque.

Supuesto:
El coalescing implementado en este proyecto es soportado para ambos lados, es decir, une regiones de memoria contiguas para la izquierda, y para la derecha.

### REALLOC
___

Supuestos de realloc:

- Cuando la región pedida es menor a la que ya está alocada,
no hace nada.
- Cuando la región pedida es mayor a la que ya está alocada, 
se copia el contenido en una nueva región, **sin perder la primera región.**

### FREE
___

Magic number:

Para validar la liberación de un puntero correcto (previamente alocado) se utiliza un número arbitrario como último atributo del struct región. Se corrobora
que éste sea el correcto cada vez que se llama a la función free.

Debido a que free no setea errno cuando falla, decidimos no implementar pruebas para este feature. Queda en responsabilidad del usuario el buen uso de la función.
