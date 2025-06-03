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
cmake --build build
```

**Opcional (compilación más rápida)**: Puedes usar múltiples núcleos de tu procesador añadiendo la opción `-j N` al comando cmake (donde N es el número de núcleos, por ejemplo, `-j 8`):

```bash
cmake --build build -j 8
```

## 3. Soporte Opcional para PortAudio

### Opción A: Modificar CMakeLists.txt (método manual)

Si necesitas los programas que utilizan PortAudio (como paplay, paudition, paview, etc.):

1. Edita el archivo `CMakeLists.txt` que se encuentra en la raíz del proyecto.
2. Busca la línea que dice:
   ```cmake
   option(USE_LOCAL_PORTAUDIO "Build and use PA programs" OFF)
   ```
3. Cámbiala a:
   ```cmake
   option(USE_LOCAL_PORTAUDIO "Build and use PA programs" ON)
   ```
4. Guarda el archivo `CMakeLists.txt`.

### Opción B: Configurar desde línea de comandos (método recomendado)

Alternativamente, puedes activar PortAudio directamente desde la línea de comandos sin modificar archivos:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DUSE_LOCAL_PORTAUDIO=ON
cmake --build build
```

### Importante sobre PortAudio

- **Si USE_LOCAL_PORTAUDIO está OFF**: El proyecto compilará sin problemas, omitiendo los programas que requieren PortAudio.
- **Si USE_LOCAL_PORTAUDIO está ON**: CMake buscará PortAudio en tu sistema. Si no lo encuentra, la compilación fallará con un mensaje claro indicándote que instales PortAudio (`brew install portaudio`) o desactives la opción.

> **Nota**: Si no necesitas estos módulos de audio en tiempo real, puedes omitir esta sección completamente. El proyecto funciona perfectamente sin PortAudio.

## 4. Estructura del Proyecto

Una vez compilado, encontrarás la siguiente estructura de directorios relevante:

- **`dev/`** — Contiene el código fuente principal del proyecto.
- **`libaaio/`** — Es una biblioteca auxiliar que se descomprime automáticamente durante el proceso.
- **`build/`** — Es la carpeta donde se guardan los archivos generados durante la compilación.
- **`build/Release/`** — Aquí se encuentran los binarios (ejecutables) finales generados.
- **`externals/portaudio/`** — Contiene las cabeceras necesarias si compilas con soporte para PortAudio.

## 5. Solución de Problemas

### Error relacionado con PortAudio
Si ves un error que menciona que PortAudio no se encuentra:

1. **Instalar PortAudio**: `brew install portaudio`
2. **O desactivar PortAudio**: Reconfigurar con `cmake -B build -DCMAKE_BUILD_TYPE=Release -DUSE_LOCAL_PORTAUDIO=OFF`

### Limpiar configuración anterior
Si necesitas empezar desde cero:

```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```