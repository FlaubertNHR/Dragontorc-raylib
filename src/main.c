#include "raylib.h"
#include <math.h>

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
float desfaseFlotando = 0.0f;

typedef enum { MURO_IZQUIERDO, MURO_FONDO, MURO_DERECHO } TipoMuro;
typedef enum { PUERTA_ABIERTA, PUERTA_CERRADA, SALIDA_LIBRE } EstadoPuerta;

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
  EstadoPuerta estado; // Registra si está abierta, cerrada o si es solo un
                       // hueco sin puerta
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
  InitWindow(screenWidth, screenHeight, "PRUEBA DE COMPILACION - NUEVO CODIGO");
  SetTargetFPS(60);

  // --- ENLAZANDO LAS PUERTAS DEL MAPA EN PAREJAS STRICTAS ---
  Sala mapa[] = {
      {360.0f,
       220.0f,
       140.0f,
       RED,
       "CAVERNS OF DOOM",
       {
           {10, MURO_FONDO, 180.0f, 60.0f, 85.0f, RED, 2, 30,
            PUERTA_CERRADA}, // Conecta con Sala 2 (Fondo)
           {11, MURO_DERECHO, 60.0f, 35.0f, 85.0f, MAROON, 1, 20,
            PUERTA_ABIERTA}, // Conecta con Sala 1
           {12, MURO_DERECHO, 160.0f, 35.0f, 85.0f, MAROON, 1, 21,
            SALIDA_LIBRE} // Salida Libre sin puerta
       },
       3},
      {240.0f,
       220.0f,
       140.0f,
       GREEN,
       "GATEHOUSE LEVEL",
       {{20, MURO_IZQUIERDO, 60.0f, 35.0f, 80.0f, MAROON, 0, 11,
         PUERTA_ABIERTA},
        {21, MURO_IZQUIERDO, 160.0f, 35.0f, 80.0f, MAROON, 0, 12,
         PUERTA_ABIERTA},
        {22, MURO_DERECHO, 110.0f, 35.0f, 80.0f, RED, 2, 31, PUERTA_ABIERTA}},
       3},
      {560.0f,
       220.0f,
       140.0f,
       BLUE,
       "THE CROSSROADS",
       {{30, MURO_FONDO, 280.0f, 50.0f, 80.0f, RED, 0, 10, PUERTA_ABIERTA},
        {31, MURO_FONDO, 80.0f, 50.0f, 80.0f, RED, 1, 22, PUERTA_ABIERTA}},
       2}};
  int salaActual = 0;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    Sala s = mapa[salaActual];

    // 1. GUARDAR LA POSICIÓN ANTES DEL MOVIMIENTO
    float antiguoX = jugadorX;
    float antiguoY = jugadorY;

    // 2. CAPTURAR EL INPUT DEL USUARIO (X REDUCIDA Y DIAGONALES CALIBRADAS)
    float movX = 0.0f;
    float movY = 0.0f;

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
      movX += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
      movX -= 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
      movY += 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
      movY -= 1.0f;

    if (movX != 0.0f || movY != 0.0f) {
      if (movX != 0.0f && movY != 0.0f) {
        movX *= 0.55f;
      }

      float longitud = sqrtf(movX * movX + movY * movY);
      movX /= longitud;
      movY /= longitud;

      float pasoX = movX * (velocidad * 0.65f) * dt;
      float pasoY = movY * velocidad * dt;

      jugadorX += pasoX;
      jugadorY += pasoY;
    }

    // SIMULACIÓN DE IMPACTO (Abre puertas cerradas en cualquiera de los muros)
    if (IsKeyPressed(KEY_SPACE)) {
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->estado == PUERTA_CERRADA) {
          if (p->muro == MURO_FONDO && jugadorY > (s.largo - 30.0f)) {
            if (jugadorX >= (p->posicion - p->ancho / 2.0f) &&
                jugadorX <= (p->posicion + p->ancho / 2.0f)) {
              p->estado = PUERTA_ABIERTA;
            }
          } else if (p->muro == MURO_IZQUIERDO && jugadorX < 30.0f) {
            if (jugadorY >= (p->posicion - p->ancho / 2.0f) &&
                jugadorY <= (p->posicion + p->ancho / 2.0f)) {
              p->estado = PUERTA_ABIERTA;
            }
          } else if (p->muro == MURO_DERECHO && jugadorX > (s.ancho - 30.0f)) {
            if (jugadorY >= (p->posicion - p->ancho / 2.0f) &&
                jugadorY <= (p->posicion + p->ancho / 2.0f)) {
              p->estado = PUERTA_ABIERTA;
            }
          }
        }
      }
    }

    // 3. SISTEMA DE COLISIONES DINÁMICAS CONTRA MUROS CIEGOS Y PUERTAS
    bool cambiadoDeSala = false;
    Puerta puertaCruzada;

    // --- COLISIÓN EN MURO IZQUIERDO (X < 0) ---
    if (jugadorX < 0) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->muro == MURO_IZQUIERDO) {
          float margenCentralY = p->ancho / 4.0f;
          if (jugadorY >= (p->posicion - margenCentralY) &&
              jugadorY <= (p->posicion + margenCentralY)) {
            if (p->estado == PUERTA_CERRADA) {
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorX < -3.0f) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          } else if (jugadorY >= (p->posicion - p->ancho / 2.0f) &&
                     jugadorY <= (p->posicion + p->ancho / 2.0f)) {
            if (p->estado == PUERTA_ABIERTA)
              p->estado = PUERTA_CERRADA;
          }
        }
      }
      if (!enHuecoPuerta)
        jugadorX = 0.0f;
    }

    // --- COLISIÓN EN MURO DERECHO (X > s.ancho) ---
    if (jugadorX > s.ancho && !cambiadoDeSala) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->muro == MURO_DERECHO) {
          float margenCentralY = p->ancho / 4.0f;
          if (jugadorY >= (p->posicion - margenCentralY) &&
              jugadorY <= (p->posicion + margenCentralY)) {
            if (p->estado == PUERTA_CERRADA) {
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorX > (s.ancho + 3.0f)) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          } else if (jugadorY >= (p->posicion - p->ancho / 2.0f) &&
                     jugadorY <= (p->posicion + p->ancho / 2.0f)) {
            if (p->estado == PUERTA_ABIERTA)
              p->estado = PUERTA_CERRADA;
          }
        }
      }
      if (!enHuecoPuerta)
        jugadorX = s.ancho;
    }

    // --- COLISIÓN EN MURO DEL FONDO (Y > s.largo) ---
    if (jugadorY > s.largo && !cambiadoDeSala) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->muro == MURO_FONDO) {
          float margenCentralX = p->ancho / 4.0f;
          if (jugadorX >= (p->posicion - margenCentralX) &&
              jugadorX <= (p->posicion + margenCentralX)) {
            if (p->estado == PUERTA_CERRADA) {
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorY > (s.largo + 8.0f)) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          } else if (jugadorX >= (p->posicion - p->ancho / 2.0f) &&
                     jugadorX <= (p->posicion + p->ancho / 2.0f)) {
            if (p->estado == PUERTA_ABIERTA)
              p->estado = PUERTA_CERRADA;
          }
        }
      }
      if (!enHuecoPuerta)
        jugadorY = s.largo;
    }
    // --- COLISIÓN EN EL FRENTE DE LA SALA (Y < 0) ---
    if (jugadorY < 0)
      jugadorY = antiguoY;
    // 4. COMPENSACIÓN ÓPTICA DEL ABANICO (SOLO SI EL MOVIMIENTO FUE VÁLIDO)
    if (movY != 0.0f && jugadorY != antiguoY) {
      float profundidadVisualFija = 220.0f;
      float pasoY = (jugadorY - antiguoY);
      float centroSala = s.ancho / 2.0f;
      float desviacion = jugadorX - centroSala;
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
          if (destino.muro == MURO_IZQUIERDO) {
            jugadorX = 3.0f;
            jugadorY = destino.posicion;
          } else if (destino.muro == MURO_DERECHO) {
            jugadorX = proximaSala.ancho - 3.0f;
            jugadorY = destino.posicion;
          } else if (destino.muro == MURO_FONDO) {
            jugadorX = destino.posicion;
            jugadorY = proximaSala.largo - 12.0f;
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

    // (Tu código original de renderizado de la habitación se queda tal cual)
    // --- RENDER DE LAS PUERTAS LEYENDO DEL MAPA REAL ---
    for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
      Puerta p = mapa[salaActual].puertas[i];
      if (p.estado == SALIDA_LIBRE) {
        if (p.muro == MURO_FONDO) {
          Vector2 basePuerta =
              ProyeccionGeometriaSala(p.posicion, s.largo, s.ancho);
          basePuerta.x += scroll2DX;
          basePuerta.y += scroll2DY;
          DrawRectangle(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                        p.ancho, p.alto, BLACK);
          DrawRectangleLinesEx((Rectangle){basePuerta.x - p.ancho / 2.0f,
                                           basePuerta.y - p.alto, p.ancho,
                                           p.alto},
                               2.0f, p.color);
        } else if (p.muro == MURO_IZQUIERDO || p.muro == MURO_DERECHO) {
          float yFrente = p.posicion - (p.ancho / 2.0f);
          float yFondo = p.posicion + (p.ancho / 2.0f);
          float xMuro = (p.muro == MURO_IZQUIERDO) ? 1.0f : s.ancho - 1.0f;
          Vector2 baseFrente = ProyeccionGeometriaSala(xMuro, yFrente, s.ancho);
          Vector2 baseFondo = ProyeccionGeometriaSala(xMuro, yFondo, s.ancho);
          baseFrente.x += scroll2DX;
          baseFrente.y += scroll2DY;
          baseFondo.x += scroll2DX;
          baseFondo.y += scroll2DY;
          Vector2 techoFrente = {baseFrente.x, baseFrente.y - p.alto};
          DrawTriangle(baseFrente, baseFondo, techoFrente, BLACK);
          DrawTriangle(techoFrente, baseFondo,
                       (Vector2){baseFondo.x, baseFondo.y - p.alto}, BLACK);
          DrawLineEx(baseFrente, techoFrente, 2.0f, p.color);
          DrawLineEx(techoFrente, (Vector2){baseFondo.x, baseFondo.y - p.alto},
                     2.0f, p.color);
          DrawLineEx((Vector2){baseFondo.x, baseFondo.y - p.alto}, baseFondo,
                     2.0f, p.color);
        }
        continue;
      }
      Color colorInterior = (p.estado == PUERTA_CERRADA) ? DARKGRAY : BLACK;
      if (p.muro == MURO_FONDO) {
        Vector2 basePuerta =
            ProyeccionGeometriaSala(p.posicion, s.largo, s.ancho);
        basePuerta.x += scroll2DX;
        basePuerta.y += scroll2DY;
        DrawRectangle(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                      p.ancho, p.alto, colorInterior);
        DrawRectangleLinesEx((Rectangle){basePuerta.x - p.ancho / 2.0f,
                                         basePuerta.y - p.alto, p.ancho,
                                         p.alto},
                             2.0f, p.color);
        if (p.estado == PUERTA_CERRADA) {
          DrawText("X", basePuerta.x - 5, basePuerta.y - p.alto / 2.0f - 5, 12,
                   RED);
        }
        if (p.estado == PUERTA_ABIERTA) {
          // 1. Fondo negro del túnel para dar profundidad
          DrawRectangle(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                        p.ancho, p.alto, BLACK);
          DrawRectangleLinesEx((Rectangle){basePuerta.x - p.ancho / 2.0f,
                                           basePuerta.y - p.alto, p.ancho,
                                           p.alto},
                               2.0f, p.color);

          // 2. NUEVA CONFIGURACIÓN: Fijamos las bisagras en el marco IZQUIERDO
          // del arco
          Vector2 bisagraAbajo = {basePuerta.x - p.ancho / 2.0f, basePuerta.y};
          Vector2 bisagraArriba = {basePuerta.x - p.ancho / 2.0f,
                                   basePuerta.y - p.alto};

          // 3. PROYECTAMOS HACIA LA IZQUIERDA (Restamos en lugar de sumar)
          // Multiplicamos por p.ancho * 0.75f hacia la izquierda para que
          // invada el suelo abriéndose de par en par
          Vector2 extremoAbajo = {basePuerta.x - p.ancho / 2.0f -
                                      p.ancho * 0.75f,
                                  basePuerta.y + p.alto * 0.25f};
          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          // Rellenamos el tablón de negro para que tape las líneas de la
          // cuadrícula del suelo
          DrawTriangle(bisagraAbajo, bisagraArriba, extremoAbajo, BLACK);
          DrawTriangle(bisagraArriba, extremoArriba, extremoAbajo, BLACK);

          // Trazamos el contorno brillante con el color original de la puerta
          // (p.color)
          DrawLineEx(bisagraAbajo, extremoAbajo, 2.5f, p.color);
          DrawLineEx(extremoAbajo, extremoArriba, 2.5f, p.color);
          DrawLineEx(extremoArriba, bisagraArriba, 2.5f, p.color);
        }

      } else if (p.muro == MURO_IZQUIERDO) {
        float yFrente = p.posicion - (p.ancho / 2.0f);
        float yFondo = p.posicion + (p.ancho / 2.0f);
        Vector2 baseFrente = ProyeccionGeometriaSala(1.0f, yFrente, s.ancho);
        Vector2 baseFondo = ProyeccionGeometriaSala(1.0f, yFondo, s.ancho);
        baseFrente.x += scroll2DX;
        baseFrente.y += scroll2DY;
        baseFondo.x += scroll2DX;
        baseFondo.y += scroll2DY;
        Vector2 techoFrente = {baseFrente.x, baseFrente.y - p.alto};
        Vector2 techoFondo = {baseFondo.x, baseFondo.y - p.alto};
        DrawTriangle(baseFrente, baseFondo, techoFrente, colorInterior);
        DrawTriangle(techoFrente, baseFondo, techoFondo, colorInterior);
        DrawLineEx(baseFrente, techoFrente, 2.0f, p.color);
        DrawLineEx(techoFrente, techoFondo, 2.0f, p.color);
        DrawLineEx(techoFondo, baseFondo, 2.0f, p.color);
        DrawLineEx(baseFondo, baseFrente, 2.0f, p.color);
        if (p.estado == PUERTA_CERRADA) {
          float centroX = (baseFrente.x + baseFondo.x) / 2.0f;
          float centroY = (baseFrente.y + baseFondo.y) / 2.0f - p.alto / 2.0f;
          DrawText("X", centroX - 4, centroY - 5, 12, RED);
        }
        if (p.estado == PUERTA_ABIERTA) {
          // Las bisagras se quedan fijas en el dintel trasero
          Vector2 bisagraAbajo = baseFondo;
          Vector2 bisagraArriba = techoFondo;

          // --- GEOMETRÍA ORIGINAL: HORIZONTAL PURA Y COMPACTA ---
          // Reducimos la anchura de la hoja al 40% de la altura para que sea
          // más pequeña
          float anchoHojaPixeles = p.alto * 0.70f;

          Vector2 extremoAbajo;
          // Al abrirse en paralelo al fondo, se desplaza en HORIZONTAL PERFECTA
          // hacia la derecha
          extremoAbajo.x = baseFondo.x + anchoHojaPixeles;
          extremoAbajo.y =
              baseFondo.y; // Mantenemos exactamente la misma Y de la bisagra

          // El extremo superior se eleva verticalmente de forma recta
          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          // Rellenamos el cuerpo en negro y trazamos sus líneas de color
          // brillante
          DrawTriangle(bisagraAbajo, bisagraArriba, extremoAbajo, BLACK);
          DrawTriangle(bisagraArriba, extremoArriba, extremoAbajo, BLACK);

          DrawLineEx(bisagraAbajo, extremoAbajo, 2.5f, p.color);
          DrawLineEx(extremoAbajo, extremoArriba, 2.5f, p.color);
          DrawLineEx(extremoArriba, bisagraArriba, 2.5f, p.color);
        }

      } else if (p.muro == MURO_DERECHO) {
        float yFrente = p.posicion - (p.ancho / 2.0f);
        float yFondo = p.posicion + (p.ancho / 2.0f);
        Vector2 baseFrente =
            ProyeccionGeometriaSala(s.ancho - 1.0f, yFrente, s.ancho);
        Vector2 baseFondo =
            ProyeccionGeometriaSala(s.ancho - 1.0f, yFondo, s.ancho);
        baseFrente.x += scroll2DX;
        baseFrente.y += scroll2DY;
        baseFondo.x += scroll2DX;
        baseFondo.y += scroll2DY;
        Vector2 techoFrente = {baseFrente.x, baseFrente.y - p.alto};
        Vector2 techoFondo = {baseFondo.x, baseFondo.y - p.alto};
        DrawTriangle(baseFrente, techoFrente, baseFondo, colorInterior);
        DrawTriangle(techoFrente, techoFondo, baseFondo, colorInterior);
        DrawLineEx(baseFrente, techoFrente, 2.0f, p.color);
        DrawLineEx(techoFrente, techoFondo, 2.0f, p.color);
        DrawLineEx(techoFondo, baseFondo, 2.0f, p.color);
        DrawLineEx(baseFondo, baseFrente, 2.0f, p.color);
        if (p.estado == PUERTA_CERRADA) {
          float centroX = (baseFrente.x + baseFondo.x) / 2.0f;
          float centroY = (baseFrente.y + baseFondo.y) / 2.0f - p.alto / 2.0f;
          DrawText("X", centroX - 4, centroY - 5, 12, RED);
        }
        if (p.estado == PUERTA_ABIERTA) {
          Vector2 bisagraAbajo = baseFondo;
          Vector2 bisagraArriba = techoFondo;

          float anchoHojaPixeles = p.alto * 0.70f;

          Vector2 extremoAbajo;
          // Espejo simétrico para el muro derecho: horizontal perfecta
          // desplazada hacia la izquierda
          extremoAbajo.x = baseFondo.x - anchoHojaPixeles;
          extremoAbajo.y = baseFondo.y; // Mantenemos la misma Y de la bisagra

          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          DrawTriangle(bisagraAbajo, extremoAbajo, bisagraArriba, BLACK);
          DrawTriangle(bisagraArriba, extremoAbajo, extremoArriba, BLACK);

          DrawLineEx(bisagraAbajo, extremoAbajo, 2.5f, p.color);
          DrawLineEx(extremoAbajo, extremoArriba, 2.5f, p.color);
          DrawLineEx(extremoArriba, bisagraArriba, 2.5f, p.color);
        }
      }
    }
    // --- DIBUJADO DEL PERSONAJE FLOTANTE Y SU SOMBRA BLANCA ---
    Vector2 posSueloPantalla = {jPosProyectada.x + scroll2DX,
                                jPosProyectada.y + scroll2DY};
    DrawEllipse(posSueloPantalla.x, posSueloPantalla.y, 16.0f, 8.0f,
                (Color){255, 255, 255, 85});
    tiempoLevitacion += dt * 4.0f;
    desfaseFlotando = sinf(tiempoLevitacion) * 3.5f;
    float alturaVisualTotal = jugadorZ + desfaseFlotando;
    float cAncho = 20.0f;
    float cAlto = 35.0f;
    DrawRectangle(posSueloPantalla.x - cAncho / 2.0f,
                  posSueloPantalla.y - cAlto - alturaVisualTotal, cAncho, cAlto,
                  WHITE);
    // [AQUÍ DIBUJAS LA INTERFAZ / HUD DE TEXTO AMARILLO]
    // DrawText(s.nombre, ...);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}