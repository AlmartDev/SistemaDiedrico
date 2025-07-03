<p align="center">
<img src="https://github.com/AlmartDev/SistemaDiedrico/blob/main/assets/favicon.ico">
</p>
<h1 align="center">Documentación</h1>

> v0.1

> [!WARNING]  
> Esta documentación ha sido creada para la version 0.16.5

## Índice
1. Introducción
2. Guia de Uso
    - Control Básico
    - Configuración
    - Creación de Elementos
    - Manipulación de Elementos
3. Instalación
    - Versión Web
    - Versión de Escritorio
    - Compilar desde código fuente
4. Hoja de Ruta

## Documentación
<details>

<summary>1. Introducción</summary>

Este proyecto trata de hacer el proceso de visualización del sistema diedrico más sencillo dibujando tanto una visualización interactiva en 3 dimensiones y su representación en sistema diedrico.
Este proyecto es completamente gratuito y código abierto: (![GitHub](https://github.com/AlmartDev/SistemaDiedrico)).
Esta aplicación lleva en desarollo irregular por Alonso Martínez desde el 13 de Abril del 2025.

</details>

<details>

<summary>2. Guia de Uso</summary>

> El uso de este programa se hace bajo una licencia propia donde queda prohibido el uso de este programa para usos comerciales y otras normas. Más información: ![Licencia Completa](https://github.com/AlmartDev/SistemaDiedrico/blob/main/LICENSE)

## 2.1 Control Básico
### Control Básico
- Para mover la cámara al rededor de la escena: ```CLICK DERECHO``` + Ratón
- Zoom en la escena: Rueda del Ratón
- Ventanas:
    - Colapsar la ventana usando la flecha en la parte superior derecha
    - Mover Cualquier ventana moviendo el Ratón desde la parte superior de cada ventana
    - Cambiar el tamaño de cualquier ventana moviendo el Ratón desde cualquiera de los laterales o esquinas de la ventana (Incluyendo la ventana de representación del diédrico)

- Reestablecer valores a por defecto:
    - Escena: Aplicación -> Reestablecer Escena
    - Cámara: Aplicación -> Reestablecer Cámara
    - Configuración: Aplicación -> Reestablecer Configuración

- Salir del Programa (**Solo versión escritorio**): Aplicación -> Salir

### Cambiar Idioma
Los únicos idiomas disponibles actualmente son Español o Inglés
- Aplicación (App) -> Idioma (Language) -> ES (Español) o EN (Inglés)

### Guardar/ Abrir Proyectos
El programa permite guardar y después cargar elementos de una escena para usarlos posteriormente o desde otro lugar.
- Guardar: Archivo -> Guardar (o ```Ctrl+S``` en la versión de Escritorio)
- Abrir: Archivo -> Abrir (o ```Ctrl+O``` en la versión de Escritorio)

## 2.2 Configuración
La primera ventana en la esquina superior izquierda es la que se encarga de la configuración del programa. No es necesario ninguna configuración para empezar a usar el prgrama.

Descripción de las opciones disponibles:
- Tipo de Ejes: Cambia el tipo de eje que se encuentra en el origen de la escena, desde ejes 3D hasta los ejes cartesianos y los dos planos que definen los diedros.
- Identificar Diedrios: Permite identificar los cuadrantes (También se puede usar presionando ```SHIFT IZQUIERDO```)
- Sensibilidad del Ratón: Cambia la velocidad con la cual rota la escena
- Offset: Cambia el centro de la escena horizontalmente o verticalmente segun sea necesario
- Escala: Cambia el sistema de referencia (Por defecto en 50 ud desde el origen hasta el final del plano)
- Opciones Extra:
    - Invertir Ratón: Invierte la dirección en la que rota la escena
    - VSync: Limita los fotogramas por segundo al límite de la pantalla para prevenir su desgarro.
    - Color del Fondo: Fondo de la escena 3D
    - Color del Diedrico: Fondo de la ventana de representación de diedrico
    - Color de Línea: Color de las líneas de la ventana de diedrico (necesario para dar contraste)

## 2.3 Creación de Elementos
Para crear elementos en la escena se usa la ventana "Escena" y se selecciona la pestaña necesaria
### Puntos
Para crear un punto es necesario introducir un nombre y escribir las coordenadas. Se pueden mostrar las trazas de los puntos usando la checkbox al lado del botón para añadir el punto en cuestion
### Líneas
Para crear una línea es necesario tener al menos 2 puntos en la escena que definan la línea, una vez la línea esta creada, modificar las coordenadas de esos puntos tambíen modificar la línea.

Tambíen se pueden mostrar las trazas si es necesario aunque puede dificultar la ligiblidad de la escena
### Planos
Al igual que las líneas, es necesario tener al menos 3 puntos en la escena y modificar cualquiera de esos 3 puntos también modificara el plano.

## 2.4 Manipulación de Elementos
### Interfaz
En la pestaña de puntos se puede modificar cualquiera de las 3 coordenadas de es punto haciendo ```CLICK IZQUIERDO``` sobre una coordenada y mover el Ratón horizontalmente.

Para manipular líneas o planos se deben mostrar los puntos que definen esos elementos usando la opción "Mostrar" en la lista de los elementos.
Modificando las coordenadas de los puntos que definen esos elementos también modificaran los elementos en si.

### Guizmos (BETA)
Alternativamente, en cualquier lista de elementos, al seleccionar un elemento haciendo ```CLICK IZQUIERDO``` en su nombre mostrará un guizmo que permite modificar su posición más intuitivamente.

</details>

<details>
<summary>3. Instalación</summary>

### 3.1 Versión Web
La versión web es identica a la versión de escritorio y se puede acceder a la última versión disponible desde este link (en Español): https://almartdev.github.io/SistemaDiedrico/

### 3.2 Versión de Escritorio
La versión de escritorio (Windows 7/8/10/11, Linux, MacOS) estará disponible para ser descargada en el momento de publicación del programa (versión 1.0) junto a un instalador. En este momento la única forma de acceder a la versión de escritorio es compilando el programa desde el código fuente

### 3.3 Compilar desde Código Fuente
El código fuente de este programa está disponible en (![GitHub](https://github.com/AlmartDev/SistemaDiedrico))

#### Requisitos:
- CMake ≥ 3.14
- Cualquier compilador de C++17
- Python 3 (para GLAD)

#### Compilación en Linux/MacOS:
```bash
git clone https://github.com/almartdev/sistemadiedrico.git
cd sistemadiedrico
mkdir build && cd build
cmake ..
make
# Run with: ./diedrico
```

#### Compilación en Windows:
```bash
git clone https://github.com/almartdev/sistemadiedrico.git
cd sistemadiedrico
mkdir build
cd build
cmake ..    
```

</details>

<details>
<summary>4. Hoja de Ruta</summary>

Cambios previstos antes de la publicación completa (1.0.0)
- Puntos de Intersección entre Planos/Líneas/Puntos
- Añadir más funcionalidades del sistema diedrico
- Reescrivir la forma de dibujar planos en diedrico

> Para proponer nuevos cambios puedes crear un issue en GitHub o enviar un correo a ```almartdev@proton.me```

</details>