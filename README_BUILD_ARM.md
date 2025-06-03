# Construcción de CDP8 en macOS ARM (Apple Silicon)

Requisitos
•	CMake ≥ 3.16
•	Xcode Command Line Tools
•	PortAudio (solo requerido si usas los módulos paprogs)
	
```brew install cmake portaudio```

## Instrucciones de compilación

````
git clone https://github.com/cjitter/CDP8-arm.git
cd CDP8-arm
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
````

Opcional: puedes añadir -jN para compilar en paralelo (N = núcleos, e.g. -j4).

### Soporte para PortAudio

PortAudio solo es necesario si deseas compilar y usar los programas paplay, pvplay, recsf, etc., que requieren captura/reproducción de audio en tiempo real. Si no planeas usar estos módulos, puedes ignorar su instalación.

#### Estructura destacada

•	dev/ — código fuente principal
•	libaaio/ — biblioteca auxiliar descomprimida automáticamente
•	build/ — carpeta de salida de compilación
•	Release/ — binarios generados