* El archivo comand-menu.sh ejecuta un script con un menu para los comandos.

* Recuerda darle permisos de ejecucion: chmod +x ./comand-menu.sh

# Comandos de ejecución

##  Generar los archivos *.CPP y *.h 
antlr4 -no-listener -visitor -Dlanguage=Cpp -o src HaploRust.g4

## Preparar los archivos de construccion
cmake -S src -B build

## Compilar el proyecto en el directorio build
cmake --build build

## Ejecutar el programa C++
build/prog      or      build/prog > output.ll