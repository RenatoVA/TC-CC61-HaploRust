#!/bin/bash

# Bucle infinito hasta que el usuario elija salir
while true; do
    # Imprimir el menú de opciones
    echo "Seleccione una opción:"
    echo "1) antlr4 -no-listener -visitor -Dlanguage=Cpp -o src HaploRust.g4"
    echo "2) cmake -S src -B build"
    echo "3) cmake --build build"
    echo "4) build/prog"
    echo "5) Salir"

    # Leer la opción ingresada por el usuario
    read -p "Opción: " option

    # Ejecutar el comando correspondiente a la opción seleccionada
    case $option in
        1)
            antlr4 -no-listener -visitor -Dlanguage=Cpp -o src HaploRust.g4
            ;;
        2)
            cmake -S src -B build
            ;;
        3)
            cmake --build build
            ;;
        4)
            echo "Opción no implementada"
            ;;
        5)
            echo "Saliendo..."
            exit 0
            ;;
        *)
            echo "Opción no válida. Por favor, seleccione una opción del 1 al 5."
            ;;
    esac

    echo # Imprimir una línea en blanco para mejorar la legibilidad del menú
done
