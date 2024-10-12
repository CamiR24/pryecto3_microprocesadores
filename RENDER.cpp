#include <stdio.h>

#define ANCHO_PANTALLA 100
#define ALTO_PANTALLA 50

// Enumeración para los posibles estados de las naves
typedef enum {
    MOSTRAR = 0,
    EXPLOSION = 1,
    NO_MOSTRAR = 2
} EstadoNave;

// Struct para definir el estado del juego
typedef struct {
    int vidasJugador;
    int puntuacion;
    EstadoNave estadoNavesEnemigas[10];  // 10 naves enemigas
    int posicionNaveJugador;               // Posición en X de la nave del jugador
    int alturaNaveJugador;                 // Posición en Y de la nave del jugador
    int posicionesNavesEnemigas[10];      // Posiciones de las naves enemigas (X)
    int alturasNavesEnemigas[10];          // Alturas de las naves enemigas (Y)
    int proyectilesJugador;
} GameState;

// Función para dibujar los límites de la pantalla
void limitesPantalla() {
    // Dibujar la línea superior
    for (int i = 0; i < ANCHO_PANTALLA; i++) {
        printf("-");
    }
    printf("\n");

    // Dibujar los lados verticales
    for (int i = 0; i < ALTO_PANTALLA - 2; i++) {
        printf("|");
        for (int j = 0; j < ANCHO_PANTALLA - 2; j++) {
            printf(" ");
        }
        printf("|\n");
    }

    // Dibujar la línea inferior
    for (int i = 0; i < ANCHO_PANTALLA; i++) {
        printf("-");
    }
    printf("\n");
}

// Función para asegurarse de que las posiciones están dentro de los límites
int dentroDeLimites(int x, int y) {
    // Verifica que x esté dentro de los límites y que y esté dentro del rango de altura
    return (x >= 1 && x <= ANCHO_PANTALLA - 5 && y >= 1 && y <= ALTO_PANTALLA - 1); // 5 es el ancho de la nave
}

// Función para mostrar la nave del jugador (diferentes niveles)
void renderNaveJugador(EstadoNave estado, int nivel, int posicion, int altura) {
    // Solo renderizar la nave si está en los límites de la pantalla
    if (dentroDeLimites(posicion, altura)) {
        switch (estado) {
            case MOSTRAR:
                if (nivel <= 2) {
                    printf("  A  \n");
                    printf(" / | \\\n");
                } else {
                    printf("   A\n");
                    printf(" < ( ) >\n");
                    printf("   /\\\n");
                }
                break;
            case EXPLOSION:
                printf("  \\ | /\n");
                printf("   — —\n");
                printf("  / | \\\n");
                break;
            case NO_MOSTRAR:
                break;
        }
    } else {
        printf("Posición de la nave del jugador fuera de los límites de la pantalla.\n");
    }
}

// Función para mostrar las naves enemigas según su estado y nivel
void renderNavesEnemigas(EstadoNave estado[], int posiciones[], int alturas[], int cantidad, int niveles[]) {
    for (int i = 0; i < cantidad; i++) {
        // Verificar si la nave enemiga está dentro de los límites de la pantalla
        if (dentroDeLimites(posiciones[i], alturas[i])) {
            switch (estado[i]) {
                case MOSTRAR:
                    if (niveles[i] == 1) {
                        printf("<00>\n");
                    } else if (niveles[i] == 2) {
                        printf("<000>\n");
                    } else if (niveles[i] == 3) {
                        printf("<0000>\n");
                    } else if (niveles[i] == 4) {
                        printf("  _A_\n");
                        printf(" / o \\\n");
                        printf(" \\_|_/\n");
                    }
                    break;
                case EXPLOSION:
                    printf("  \\ | /\n");
                    printf("   — —\n");
                    printf("  / | \\\n");
                    break;
                case NO_MOSTRAR:
                    break;
            }
        } else {
            printf("Nave enemiga en posición %d, altura %d está fuera de los límites de la pantalla.\n", posiciones[i], alturas[i]);
        }
    }
}

// Función para mostrar proyectiles
void renderProyectiles(int proyectilesJugador) {
    for (int i = 0; i < proyectilesJugador; i++) {
        printf("|\n");
    }
}

// Función principal de renderizado
void render(GameState *estado) {
    
    // Dibujar los límites de la pantalla
    limitesPantalla();
    
    // Renderizar información del jugador
    printf("Vidas del jugador: %d\n", estado->vidasJugador);
    printf("Puntuación: %d\n", estado->puntuacion);

    // Renderizar la nave del jugador
    renderNaveJugador(estado->estadoNavesEnemigas[0], 2, estado->posicionNaveJugador, estado->alturaNaveJugador); // Usando un nivel de 2 y altura de la nave del jugador

    // Renderizar las naves enemigas
    renderNavesEnemigas(estado->estadoNavesEnemigas, estado->posicionesNavesEnemigas, estado->alturasNavesEnemigas, 10, estado->alturasNavesEnemigas);

    // Renderizar los proyectiles
    renderProyectiles(estado->proyectilesJugador);
}

// Función principal donde se inicializa el estado del juego y se llama a render
int main() {
    // Inicializar el estado del juego
    GameState estado = {
        .vidasJugador = 3,
        .puntuacion = 100,
        .estadoNavesEnemigas = {MOSTRAR, MOSTRAR, MOSTRAR, EXPLOSION, MOSTRAR, MOSTRAR, MOSTRAR, MOSTRAR, MOSTRAR, MOSTRAR},
        .posicionNaveJugador = 5,
        .alturaNaveJugador = ALTO_PANTALLA - 2, // Posición Y de la nave del jugador
        .posicionesNavesEnemigas = {10, 12, 14, 16, 18, 20, 22, 24, 26, 28},
        .alturasNavesEnemigas = {1, 2, 3, 4, 1, 1, 2, 3, 1, 4}, // Asignar alturas a las naves enemigas
        .proyectilesJugador = 1,
    };

    // Niveles de las naves enemigas (1, 2, 3 o 4 según el daño que pueden recibir)
    int nivelesEnemigos[10] = {1, 2, 3, 4, 1, 1, 2, 3, 1, 4};

    // Llamar a la función render para mostrar el estado del juego
    render(&estado);

    return 0;
}
