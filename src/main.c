#include "math.h"
#include "raylib.h"

const int screenWidth = 800;
const int screenHeight = 600;
const int TILE_SIZE = 40;

// Variables lógicas globales del jugador
float jugadorX = 180.0f;
float jugadorY = 40.0f;
float velocidad = 160.0f;
// --- NUEVAS VARIABLES DE ANIMACIÓN Y FLOTABILIDAD ---
float jugadorZ = 12.0f; // Altura fija de flotación respecto al suelo
float tiempoLevitacion =
    0.0f; // Temporizador para la oscilación suave (bobbing)

typedef enum { MURO_IZQUIERDO, MURO_FONDO, MURO_DERECHO } TipoMuro;

// --- ESTRUCTURA DE PUERTA INTERCONECTADA DE UNO A UNO ---
typedef struct {
  int id; // ID único de esta puerta
  TipoMuro muro;
  float posicion;
  float ancho;
  float alto;
  Color color;

  int salaDestino;     // Índice de la sala a la que viaja
  int idPuertaDestino; // ID de la puerta por la que va a salir en la otra sala
} Puerta;

typedef struct {
  float ancho;
  float largo;
  float alturaPared;
  Color colorMuro;
  const char *nombre;
  Puerta puertas[6]; // Capacidad máxima por sala
  int cantidadPuertas;
} Sala;

// --- PROYECCIÓN EN ABANICO RÍGIDO CONSTANTE (VALIDADA) ---
Vector2 ProyeccionGeometriaSala(float x, float y, float anchoSala) {
  Vector2 pos;
  float profundidadVisualFija = 220.0f;

  float factorY = y / profundidadVisualFija;
  if (factorY < 0.0f)
    factorY = 0.0f;
  if (factorY > 1.0f)
    factorY = 1.0f;

  float xFrenteIzquierdo = -310.0f;
  float xFondoIzquierdo = -130.0f;
  float limiteIzquierdoPantalla =
      xFrenteIzquierdo + (xFondoIzquierdo - xFrenteIzquierdo) * factorY;

  float xFrenteDerecho = anchoSala + 310.0f;
  float xFondoDerecho = anchoSala + 130.0f;
  float limiteDerechoPantalla =
      xFrenteDerecho + (xFondoDerecho - xFrenteDerecho) * factorY;

  float factorX = 0.0f;
  if (anchoSala > 0)
    factorX = x / anchoSala;

  pos.x = limiteIzquierdoPantalla +
          (limiteDerechoPantalla - limiteIzquierdoPantalla) * factorX;
  pos.y = -(y * 0.45f);

  pos.x *= 1.3f;
  pos.y *= 1.3f;

  return pos;
}

int main(void) {
  // Cambia el título de la ventana para verificar que compila el código
  // correcto
  InitWindow(screenWidth, screenHeight, "PRUEBA DE COMPILACION - NUEVO CODIGO");

  SetTargetFPS(60);

  // --- ENLAZANDO LAS PUERTAS DEL MAPA EN PAREJAS STRICTAS ---
  Sala mapa[] = {{360.0f,
                  220.0f,
                  140.0f,
                  RED,
                  "CAVERNS OF DOOM",
                  {
                      {10, MURO_FONDO, 180.0f, 60.0f, 85.0f, RED, 2,
                       30}, // Conecta con Sala 2 (Fondo)
                      {11, MURO_DERECHO, 60.0f, 35.0f, 85.0f, MAROON, 1,
                       20}, // Conecta con Sala 1 (Izq abajo)
                      {12, MURO_DERECHO, 160.0f, 35.0f, 85.0f, MAROON, 1, 21}
                      // Conecta con Sala 1 (Izq arriba)
                  },
                  3},
                 {240.0f,
                  220.0f,
                  140.0f,
                  GREEN,
                  "GATEHOUSE LEVEL",
                  {
                      {20, MURO_IZQUIERDO, 60.0f, 35.0f, 80.0f, MAROON, 0,
                       11}, // Regresa exacto a la 11 de la Sala 0
                      {21, MURO_IZQUIERDO, 160.0f, 35.0f, 80.0f, MAROON, 0,
                       12}, // Regresa exacto a la 12 de la Sala 0
                      {22, MURO_DERECHO, 110.0f, 35.0f, 80.0f, RED, 2, 31}
                      // Conecta con la Sala 2 (Fondo izq)
                  },
                  3},
                 {560.0f,
                  220.0f,
                  140.0f,
                  BLUE,
                  "THE CROSSROADS",
                  {
                      {30, MURO_FONDO, 280.0f, 50.0f, 80.0f, RED, 0,
                       10}, // Regresa exacto a la 10 de la Sala 0
                      {31, MURO_FONDO, 80.0f, 50.0f, 80.0f, RED, 1, 22}
                      // Regresa exacto a la 22 de la Sala 1
                  },
                  2}};
  int salaActual = 0;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    Sala s = mapa[salaActual];

    // 1. GUARDAR LA POSICIÓN ANTES DEL MOVIMIENTO (Esto se queda igual)
    float antiguoX = jugadorX;
    float antiguoY = jugadorY;

    // 2. CAPTURAR EL INPUT DEL USUARIO (MOVIMIENTO BASE PLANO SIN ALTERAR X EN
    // ESTE PASO)
    float movX = 0.0f;
    float movY = 0.0f;

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
      movX += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
      movX -= 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
      movY += 1.0f; // Sube hacia el fondo
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
      movY -= 1.0f; // Baja hacia el frente

    if (movX != 0.0f || movY != 0.0f) {
      // Calibración de diagonales para la X lenta (65%) que pusimos antes
      if (movX != 0.0f && movY != 0.0f) {
        movX *= 0.55f;
      }

      // Normalización del vector de entrada
      float longitud = sqrtf(movX * movX + movY * movY);
      movX /= longitud;
      movY /= longitud;

      // AVANCE LOGÍSTICO INICIAL (Calculamos el paso de forma independiente)
      float pasoX = movX * (velocidad * 0.65f) * dt;
      float pasoY = movY * velocidad * dt;

      jugadorX += pasoX;
      jugadorY += pasoY;
    }

    // 3. SISTEMA DE COLISIONES DINÁMICAS CONTRA MUROS CIEGOS
    bool cambiadoDeSala = false;
    Puerta puertaCruzada;

    // --- COLISIÓN EN MURO IZQUIERDO (X < 0) ---
    if (jugadorX < 0) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < s.cantidadPuertas; i++) {
        Puerta p = s.puertas[i];
        if (p.muro == MURO_IZQUIERDO &&
            jugadorY >= (p.posicion - p.ancho / 2.0f) &&
            jugadorY <= (p.posicion + p.ancho / 2.0f)) {
          puertaCruzada = p;
          cambiadoDeSala = true;
          enHuecoPuerta = true;
          break;
        }
      }
      if (!enHuecoPuerta)
        jugadorX = antiguoX;
    }

    // --- COLISIÓN EN MURO DERECHO (X > s.ancho) ---
    if (jugadorX > s.ancho && !cambiadoDeSala) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < s.cantidadPuertas; i++) {
        Puerta p = s.puertas[i];
        if (p.muro == MURO_DERECHO &&
            jugadorY >= (p.posicion - p.ancho / 2.0f) &&
            jugadorY <= (p.posicion + p.ancho / 2.0f)) {
          puertaCruzada = p;
          cambiadoDeSala = true;
          enHuecoPuerta = true;
          break;
        }
      }
      if (!enHuecoPuerta)
        jugadorX = antiguoX;
    }

    // --- COLISIÓN EN MURO DEL FONDO (ZONA CENTRAL ESTRICTA) ---
    if (jugadorY > s.largo && !cambiadoDeSala) {
      bool enHuecoPuerta = false;

      for (int i = 0; i < s.cantidadPuertas; i++) {
        Puerta p = s.puertas[i];
        if (p.muro == MURO_FONDO) {
          // Reducimos el ancho de colisión efectivo a la mitad (ancho / 4.0f a
          // cada lado) Así obligamos a pasar obligatoriamente por el CENTRO
          // real de la puerta
          float margenCentralX = p.ancho / 4.0f;

          if (jugadorX >= (p.posicion - margenCentralX) &&
              jugadorX <= (p.posicion + margenCentralX)) {
            enHuecoPuerta = true;

            // Activamos la transición SOLO si la sombra se adentra físicamente
            // en la profundidad del dintel (+8 píxeles del radio de la elipse)
            if (jugadorY > (s.largo + 8.0f)) {
              puertaCruzada = p;
              cambiadoDeSala = true;
            }
            break;
          }
        }
      }

      // Si no está en el centro de la puerta o choca con muro ciego, se detiene
      // en seco en el dintel
      if (!enHuecoPuerta) {
        jugadorY = s.largo;
      }
    }
    // --- COLISIÓN EN EL FRENTE DE LA SALA (Y < 0) ---
    if (jugadorY < 0)
      jugadorY = antiguoY;

    // 4. NUEVA SECCIÓN: COMPENSACIÓN ÓPTICA DEL ABANICO (SOLO SI EL MOVIMIENTO
    // FUE VÁLIDO) Si el jugador ha chocado y ha sido devuelto a antiguoY,
    // jugadorY será igual a antiguoY, por lo que no se aplicará ningún
    // desplazamiento lateral indeseado.
    if (movY != 0.0f && jugadorY != antiguoY) {
      float profundidadVisualFija = 220.0f;
      float factorY = jugadorY / profundidadVisualFija;
      if (factorY < 0.0f)
        factorY = 0.0f;
      if (factorY > 1.0f)
        factorY = 1.0f;

      float pasoY =
          (jugadorY -
           antiguoY); // Calculamos el avance vertical real neto que ha ocurrido
      float centroSala = s.ancho / 2.0f;
      float desviacion = jugadorX - centroSala;

      // Aplicamos la corrección de fuga en X solo basándonos en el movimiento
      // vertical real permitido
      jugadorX += (desviacion * (0.45f / profundidadVisualFija)) * pasoY;
    }

    // --- PROCESAMIENTO DE LA APARICIÓN EN LA PAREJA DE LA PUERTA ---
    if (cambiadoDeSala) {
      int proximaSalaIdx = puertaCruzada.salaDestino;
      int buscarID = puertaCruzada.idPuertaDestino;
      Sala proximaSala = mapa[proximaSalaIdx];

      for (int i = 0; i < proximaSala.cantidadPuertas; i++) {
        Puerta destino = proximaSala.puertas[i];
        if (destino.id == buscarID) {
          // Colocamos al jugador justo enfrente del umbral en el mapa de
          // destino dando un paso hacia el interior (18 píxeles) para evitar
          // bucles
          // Colocamos al jugador pegado al umbral en el mapa de destino
          // Reducimos a 2.0f o 3.0f para que no flote lejos del muro visual
          if (destino.muro == MURO_IZQUIERDO) {
            jugadorX = 3.0f; // Ajuste drástico para aparecer pegado a la pared
                             // izquierda
            jugadorY = destino.posicion;
          } else if (destino.muro == MURO_DERECHO) {
            jugadorX = proximaSala.ancho - 3.0f; // Ajuste para aparecer pegado
                                                 // a la pared derecha
            jugadorY = destino.posicion;
          } else if (destino.muro == MURO_FONDO) {
            jugadorX = destino.posicion;
            jugadorY = proximaSala.largo - 12.0f; // El fondo de la pantalla sí
                                                  // requiere algo más de margen
          }
          break;
        }
      }
      salaActual = proximaSalaIdx;
      s = mapa[salaActual];
    }

    // SCROLL CENTRADO
    Vector2 jPosProyectada =
        ProyeccionGeometriaSala(jugadorX, jugadorY, s.ancho);
    float centroPantallaX = screenWidth / 2.0f;
    float centroPantallaY = 260.0f;
    float scroll2DX = centroPantallaX - jPosProyectada.x;
    float scroll2DY = centroPantallaY - jPosProyectada.y;

    BeginDrawing();
    ClearBackground(BLACK);

    // REJILLA
    for (float i = 0; i <= s.ancho; i += TILE_SIZE) {
      Vector2 p1 = ProyeccionGeometriaSala(i, 0, s.ancho);
      Vector2 p2 = ProyeccionGeometriaSala(i, s.largo, s.ancho);
      DrawLine(p1.x + scroll2DX, p1.y + scroll2DY, p2.x + scroll2DX,
               p2.y + scroll2DY, DARKGRAY);
    }
    for (float i = 0; i <= s.largo; i += TILE_SIZE) {
      Vector2 p1 = ProyeccionGeometriaSala(0, i, s.ancho);
      Vector2 p2 = ProyeccionGeometriaSala(s.ancho, i, s.ancho);
      DrawLine(p1.x + scroll2DX, p1.y + scroll2DY, p2.x + scroll2DX,
               p2.y + scroll2DY, DARKGRAY);
    }

    // VÉRTICES PAREDES
    Vector2 pIzqAbajo = ProyeccionGeometriaSala(0, 0, s.ancho);
    Vector2 pIzqArriba = ProyeccionGeometriaSala(0, s.largo, s.ancho);
    Vector2 pDerArriba = ProyeccionGeometriaSala(s.ancho, s.largo, s.ancho);
    Vector2 pDerAbajo = ProyeccionGeometriaSala(s.ancho, 0, s.ancho);

    pIzqAbajo.x += scroll2DX;
    pIzqAbajo.y += scroll2DY;
    pIzqArriba.x += scroll2DX;
    pIzqArriba.y += scroll2DY;
    pDerArriba.x += scroll2DX;
    pDerArriba.y += scroll2DY;
    pDerAbajo.x += scroll2DX;
    pDerAbajo.y += scroll2DY;

    // DIBUJAR PAREDES
    DrawLineV(pIzqAbajo, pIzqArriba, s.colorMuro);
    DrawLineV((Vector2){pIzqAbajo.x, pIzqAbajo.y - s.alturaPared},
              (Vector2){pIzqArriba.x, pIzqArriba.y - s.alturaPared},
              s.colorMuro);
    DrawLineV(pIzqAbajo, (Vector2){pIzqAbajo.x, pIzqAbajo.y - s.alturaPared},
              s.colorMuro);
    DrawLineV(pIzqArriba, (Vector2){pIzqArriba.x, pIzqArriba.y - s.alturaPared},
              s.colorMuro);

    DrawLineV(pIzqArriba, pDerArriba, s.colorMuro);
    DrawLineV((Vector2){pIzqArriba.x, pIzqArriba.y - s.alturaPared},
              (Vector2){pDerArriba.x, pDerArriba.y - s.alturaPared},
              s.colorMuro);
    DrawLineV(pDerArriba, (Vector2){pDerArriba.x, pDerArriba.y - s.alturaPared},
              s.colorMuro);

    DrawLineV(pDerArriba, pDerAbajo, s.colorMuro);
    DrawLineV((Vector2){pDerAbajo.x, pDerAbajo.y - s.alturaPared},
              (Vector2){pDerArriba.x, pDerArriba.y - s.alturaPared},
              s.colorMuro);
    DrawLineV(pDerAbajo, (Vector2){pDerAbajo.x, pDerAbajo.y - s.alturaPared},
              s.colorMuro);

    // RENDER DE LAS PUERTAS
    for (int i = 0; i < s.cantidadPuertas; i++) {
      Puerta p = s.puertas[i];
      if (p.muro == MURO_FONDO) {
        Vector2 basePuerta =
            ProyeccionGeometriaSala(p.posicion, s.largo, s.ancho);
        basePuerta.x += scroll2DX;
        basePuerta.y += scroll2DY;
        DrawRectangle(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                      p.ancho, p.alto, p.color);
        DrawRectangleLines(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                           p.ancho, p.alto, BLACK);
      } else if (p.muro == MURO_IZQUIERDO) {
        float yFrente = p.posicion - (p.ancho / 2.0f);
        float yFondo = p.posicion + (p.ancho / 2.0f);
        Vector2 baseFrente = ProyeccionGeometriaSala(0.0f, yFrente, s.ancho);
        Vector2 baseFondo = ProyeccionGeometriaSala(0.0f, yFondo, s.ancho);
        baseFrente.x += scroll2DX;
        baseFrente.y += scroll2DY;
        baseFondo.x += scroll2DX;
        baseFondo.y += scroll2DY;
        Vector2 techoFrente = {baseFrente.x, baseFrente.y - p.alto};
        Vector2 techoFondo = {baseFondo.x, baseFondo.y - p.alto};
        DrawTriangle(baseFrente, baseFondo, techoFrente, p.color);
        DrawTriangle(techoFrente, baseFondo, techoFondo, p.color);
        DrawLineV(baseFrente, techoFrente, BLACK);
        DrawLineV(techoFrente, techoFondo, BLACK);
        DrawLineV(techoFondo, baseFondo, BLACK);
        DrawLineV(baseFondo, baseFrente, BLACK);
      } else if (p.muro == MURO_DERECHO) {
        float yFrente = p.posicion - (p.ancho / 2.0f);
        float yFondo = p.posicion + (p.ancho / 2.0f);
        Vector2 baseFrente = ProyeccionGeometriaSala(s.ancho, yFrente, s.ancho);
        Vector2 baseFondo = ProyeccionGeometriaSala(s.ancho, yFondo, s.ancho);
        baseFrente.x += scroll2DX;
        baseFrente.y += scroll2DY;
        baseFondo.x += scroll2DX;
        baseFondo.y += scroll2DY;
        Vector2 techoFrente = {baseFrente.x, baseFrente.y - p.alto};
        Vector2 techoFondo = {baseFondo.x, baseFondo.y - p.alto};
        DrawTriangle(baseFrente, techoFrente, baseFondo, p.color);
        DrawTriangle(techoFrente, techoFondo, baseFondo, p.color);
        DrawLineV(baseFrente, techoFrente, BLACK);
        DrawLineV(techoFrente, techoFondo, BLACK);
        DrawLineV(techoFondo, baseFondo, BLACK);
        DrawLineV(baseFondo, baseFrente, BLACK);
      }
    }
    // --- DIBUJADO DEL PERSONAJE ESTILO DRAGONTORC ---
    // 1. Conseguimos la posición proyectada exacta de los "pies" (el suelo)
    Vector2 jSueloProyectado =
        ProyeccionGeometriaSala(jugadorX, jugadorY, s.ancho);

    // Aplicamos el scroll de cámara a las coordenadas proyectadas
    Vector2 posSueloPantalla = {jSueloProyectado.x + scroll2DX,
                                jSueloProyectado.y + scroll2DY};

    // Dibujamos la SOMBRA (Círculo/Óvalo de colisión e interacción)
    // Se dibuja en negro/gris translúcido justo encima de las baldosas
    DrawEllipse(posSueloPantalla.x, posSueloPantalla.y, 16.0f, 8.0f,
                GetColor(0x00000044));

    // 2. Calculamos la levitación flotante (Oscilación matemática usando Seno)
    tiempoLevitacion += dt * 4.0f; // Controla la velocidad del balanceo
    float desfaseFlotando =
        sinf(tiempoLevitacion) * 4.0f; // Amplitud del balanceo (4 píxeles)

    // La posición del cuerpo visual se desplaza hacia arriba sumando la base Z
    // + el balanceo
    float desplazamientoVisualY = jugadorZ + desfaseFlotando;

    // --- DIBUJADO DEL PERSONAJE FLOTANTE Y SU SOMBRA EN EL SUELO ---

    // 1. Calculamos la posición visual exacta de los "pies" del jugador en el
    // mapa
    jSueloProyectado = ProyeccionGeometriaSala(jugadorX, jugadorY, s.ancho);

    // Aplicamos el scroll de la cámara para centrarlo en la pantalla
    posSueloPantalla.x = jSueloProyectado.x + scroll2DX;
    posSueloPantalla.y = jSueloProyectado.y + scroll2DY;

    // -------------------------------------------------------------------------
    // DIBUJAR LA SOMBRA (El círculo físico de interacción)
    // Dibujamos un óvalo negro semi-transparente pegado a las baldosas.
    // Al pasar por encima de las líneas del suelo, se verá perfectamente.
    // -------------------------------------------------------------------------
    // Cambiamos el color a Blanco Puro (255, 255, 255)
    // Mantenemos la opacidad en 85 sobre 255 para que sea translúcida y deje
    // ver las baldosas
    DrawEllipse(posSueloPantalla.x, posSueloPantalla.y, 16.0f, 8.0f,
                (Color){255, 255, 255, 85});

    // 2. Calculamos la levitación del cuerpo (Efecto "bobbing" flotante con
    // Seno)
    tiempoLevitacion += dt * 4.0f; // Velocidad del vaivén
    desfaseFlotando =
        sinf(tiempoLevitacion) * 3.5f; // Amplitud (sube y baja 3.5 píxeles)

    // El cuerpo visual flota a una altura fija (12.0f) más el balanceo
    // matemático
    float alturaVisualTotal = jugadorZ + desfaseFlotando;

    // -------------------------------------------------------------------------
    // DIBUJAR EL CUERPO FLOTANTE (El cubo blanco)
    // Desplazamos el cubo hacia arriba usando 'alturaVisualTotal'
    // -------------------------------------------------------------------------
    float cuboAncho = 20.0f;
    float cuboAlto = 35.0f;
    DrawRectangle(posSueloPantalla.x - cuboAncho / 2.0f,
                  posSueloPantalla.y - cuboAlto - alturaVisualTotal, cuboAncho,
                  cuboAlto, WHITE);

    // INTERFAZ
    DrawRectangle(0, 450, screenWidth, 150, GOLD);
    DrawRectangleLines(10, 460, screenWidth - 20, 130, BLACK);
    DrawText("move", 60, 480, 25, BLACK);
    DrawText(s.nombre, 60, 520, 28, BLACK);
    DrawText("Cada puerta está ahora correctamente enlazada una a una.", 300,
             480, 16, DARKGRAY);
    DrawFPS(10, 10);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
