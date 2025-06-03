# Guía Simplificada para la Construcción de CDP8 en macOS arm64

Este documento proporciona una guía concisa para compilar el proyecto CDP8 en un Mac con Apple Silicon (arm64).

## 1. Requisitos Previos

Asegúrate de tener lo siguiente instalado:

- **CMake**: Versión 3.16 o superior.
- **Xcode Command Line Tools**: Se pueden instalar ejecutando `xcode-select --install` en la terminal.
- **PortAudio** (Opcional): Solo si necesitas compilar y usar los módulos paprogs que manejan audio en tiempo real (ej. paplay, pvplay).

Puedes instalar CMake y PortAudio usando Homebrew:

```bash
brew install cmake portaudio
```

## 2. Pasos de Compilación

Abre tu terminal y sigue estos pasos:

### Clonar el repositorio:
```bash
git clone https://github.com/cjitter/CDP8-arm.git
```

### Navegar al directorio del proyecto:
```bash
cd CDP8-arm
```

### Configurar el proyecto con CMake:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Compilar el código:
```bash
make --build build
```

**Opcional (compilación más rápida)**: Puedes usar múltiples núcleos de tu procesador añadiendo la opción `-jN` al comando make (donde N es el número de núcleos, por ejemplo, `-j8`):

```bash
make --build build -j8
```

## 3. Soporte Opcional para PortAudio

Si necesitas los programas que utilizan PortAudio (como paplay, paudition, paview, etc.):

1. Edita el archivo `CMakeLists.txt` que se encuentra en la raíz del proyecto.
2. Busca la línea (aproximadamente la línea 210) que dice:
   ```cmake
   option(USE_LOCAL_PORTAUDIO "Build and use PA programs" OFF)
   ```
3. Cámbiala a:
   ```cmake
   option(USE_LOCAL_PORTAUDIO "Build and use PA programs" ON)
   ```
4. Guarda el archivo `CMakeLists.txt`.
5. Vuelve a ejecutar los pasos de configuración y compilación (pasos 3 y 4 de la sección anterior).

> **Nota**: Si no necesitas estos módulos de audio en tiempo real, puedes omitir esta sección.

## 4. Estructura del Proyecto

Una vez compilado, encontrarás la siguiente estructura de directorios relevante:

- **`dev/`** — Contiene el código fuente principal del proyecto.
- **`libaaio/`** — Es una biblioteca auxiliar que se descomprime automáticamente durante el proceso.
- **`build/`** — Es la carpeta donde se guardan los archivos generados durante la compilación.
- **`externals/portaudio/`** — Contiene las cabeceras necesarias si compilas con soporte para PortAudio.
- **`Release/`** — Aquí se encuentran los binarios (ejecutables) finales generados.