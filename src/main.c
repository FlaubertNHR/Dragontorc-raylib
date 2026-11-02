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

// Variables para el rebote físico gradual
float reboteFuerzaX = 0.0f;
float reboteFuerzaY = 0.0f;
float tiempoBloqueoRebote =
    0.0f; // Si es mayor que 0, el jugador no tiene el control

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

    // 2. CAPTURAR EL INPUT DEL USUARIO (CON FÍSICAS DE REBOTE GRADUAL)
    float movX = 0.0f;
    float movY = 0.0f;

    // Si el personaje está sufriendo un rebote, reducimos el tiempo y aplicamos
    // la fuerza
    if (tiempoBloqueoRebote > 0.0f) {
      tiempoBloqueoRebote -= dt;

      // Desplazamos al jugador de forma gradual con las fuerzas acumuladas
      jugadorX += reboteFuerzaX * dt;
      jugadorY += reboteFuerzaY * dt;

      // Fricción: frenamos la fuerza progresivamente para que el rebote sea
      // suave
      reboteFuerzaX *= 0.85f;
      reboteFuerzaY *= 0.85f;
    } else {
      // Si no hay rebote activo, el jugador tiene el control normal
      if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        movX += 1.0f;
      if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        movX -= 1.0f;
      if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
        movY += 1.0f;
      if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
        movY -= 1.0f;

      if (movX != 0.0f || movY != 0.0f) {
        if (movX != 0.0f && movY != 0.0f)
          movX *= 0.55f;
        float longitud = sqrtf(movX * movX + movY * movY);
        movX /= longitud;
        movY /= longitud;

        // --- CÁLCULO DE VELOCIDAD DE PANTALLA PURA E IDÉNTICA EN CUALQUIER
        // SALA ---
        // 1. Calculamos cuánto se estira un paso de '1.0' unidad en la pantalla
        // en esta sala concreta
        Vector2 pBase = ProyeccionGeometriaSala(0.0f, jugadorY, s.ancho);
        Vector2 pTest = ProyeccionGeometriaSala(1.0f, jugadorY, s.ancho);
        float pixelesPorUnidadLogica = fabsf(pTest.x - pBase.x);

        // 2. Si el ratio visual es válido, calculamos el paso lógico exacto
        // necesario para avanzar una cantidad fija de píxeles visuales en el
        // monitor
        float pasoX = 0.0f;
        if (pixelesPorUnidadLogica > 0.0f) {
          // Modificando el 1.35f puedes cambiar la velocidad horizontal global
          // de forma uniforme
          pasoX = (movX * (velocidad * 1.35f) * dt) / pixelesPorUnidadLogica;
        }
        float pasoY = movY * velocidad * dt;

        // 3. Aplicamos exactamente la misma corrección visual adaptativa al
        // rebote gradual
        if (tiempoBloqueoRebote > 0.0f) {
          tiempoBloqueoRebote -= dt;

          float reboteLogicoX = 0.0f;
          if (pixelesPorUnidadLogica > 0.0f) {
            reboteLogicoX = (reboteFuerzaX * dt) / pixelesPorUnidadLogica;
          }

          jugadorX += reboteLogicoX;
          jugadorY += reboteFuerzaY * dt;
          reboteFuerzaX *= 0.85f;
          reboteFuerzaY *= 0.85f;
        } else {
          jugadorX += pasoX;
          jugadorY += pasoY;
        }
      }
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

    // =========================================================================
    // 3. SISTEMA DE COLISIONES DINÁMICAS, REBOTE OBLIGATORIO Y CIERRE DE HOJA
    // =========================================================================
    bool cambiadoDeSala = false;
    Puerta puertaCruzada;

    // --- COLISIÓN EN MURO IZQUIERDO (Margen preventivo de X < 8.0f) ---
    if (jugadorX < 8.0f) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->muro == MURO_IZQUIERDO) {
          float margenCentralY = p->ancho / 4.0f;

          // Entrada por el centro del dintel (Apertura o Traspaso)
          if (jugadorY >= (p->posicion - margenCentralY) &&
              jugadorY <= (p->posicion + margenCentralY)) {
            // Al impactar, inyectamos una velocidad hacia la derecha (+X)
            if (p->estado == PUERTA_CERRADA) {
              p->estado = PUERTA_ABIERTA;
              reboteFuerzaX =
                  95.0f; // Reducido a la mitad para evitar el latigazo brusco
              reboteFuerzaY = 0.0f;
              tiempoBloqueoRebote =
                  0.35f; // Extendemos ligeramente la amortiguación
              jugadorX = 0.0f;
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorX < -3.0f) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          }
        }
      }
      if (!enHuecoPuerta && jugadorX < 0.0f)
        jugadorX = 0.0f;
    }

    // --- COLISIÓN EN MURO DERECHO (Margen preventivo de X > s.ancho - 8.0f)
    // ---
    if (jugadorX > (s.ancho - 8.0f) && !cambiadoDeSala) {
      bool enHuecoPuerta = false;
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->muro == MURO_DERECHO) {
          float margenCentralY = p->ancho / 4.0f;

          if (jugadorY >= (p->posicion - margenCentralY) &&
              jugadorY <= (p->posicion + margenCentralY)) {
            // Al impactar, inyectamos velocidad hacia la izquierda (-X)
            if (p->estado == PUERTA_CERRADA) {
              p->estado = PUERTA_ABIERTA;
              reboteFuerzaX = -95.0f; // Reducido
              reboteFuerzaY = 0.0f;
              tiempoBloqueoRebote = 0.35f;
              jugadorX = s.ancho;
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorX > (s.ancho + 3.0f)) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          }
        }
      }
      if (!enHuecoPuerta && jugadorX > s.ancho)
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
            // Al impactar, inyectamos velocidad hacia abajo (-Y)
            if (p->estado == PUERTA_CERRADA) {
              p->estado = PUERTA_ABIERTA;
              reboteFuerzaX = 0.0f;
              reboteFuerzaY = -95.0f; // Reducido para un retroceso vertical
                                      // flotante y suave
              tiempoBloqueoRebote = 0.35f;
              jugadorY = s.largo;
              enHuecoPuerta = false;
            } else {
              enHuecoPuerta = true;
              if (jugadorY > (s.largo + 8.0f)) {
                puertaCruzada = *p;
                cambiadoDeSala = true;
              }
            }
            break;
          }
        }
      }
      if (!enHuecoPuerta)
        jugadorY = s.largo;
    }

    // --- NUEVA MECÁNICA DE CIERRE POR EMPUJE (MUROS LATERALES Y FONDO) ---
    // 1. Cierre de puertas laterales (Caminar hacia abajo chocando la hoja)
    if (movY < 0.0f) {
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];
        if (p->estado == PUERTA_ABIERTA) {
          float longitudHojaLogica = p->alto * 0.75f;

          if (p->muro == MURO_IZQUIERDO) {
            if (jugadorX >= 0.0f && jugadorX <= longitudHojaLogica) {
              if (antiguoY >= p->posicion && jugadorY < p->posicion) {
                p->estado = PUERTA_CERRADA;
                jugadorY = p->posicion + 8.0f;
              }
            }
          } else if (p->muro == MURO_DERECHO) {
            if (jugadorX >= (s.ancho - longitudHojaLogica) &&
                jugadorX <= s.ancho) {
              if (antiguoY >= p->posicion && jugadorY < p->posicion) {
                p->estado = PUERTA_CERRADA;
                jugadorY = p->posicion + 8.0f;
              }
            }
          }
        }
      }
    }

    // 2. Cierre de puerta del fondo (Caminar hacia la derecha chocando la zona
    // central de la hoja)
    // 2. Cierre de puerta del fondo (Caminar hacia la derecha chocando el
    // centro de la hoja)
    if (movX > 0.0f) {
      for (int i = 0; i < mapa[salaActual].cantidadPuertas; i++) {
        Puerta *p = &mapa[salaActual].puertas[i];

        if (p->muro == MURO_FONDO && p->estado == PUERTA_ABIERTA) {
          if (jugadorY >= (s.largo - 10.0f) && jugadorY <= (s.largo + 15.0f)) {

            float marcoIzquierdo = p->posicion - (p->ancho / 2.0f);

            // --- CALIBRACIÓN ESTRICTA DEL CENTRO DE LA HOJA ---
            // Reducimos la zona de impacto para centrarla de forma milimétrica.
            // Forzamos a que el personaje ignore el extremo izquierdo (el 0.75f
            // exterior) y solo active el cierre cuando el borde de la sombra
            // (jugadorX + 8.0f) muerda el segmento central de la madera roja.
            float zonaImpactoIzquierda =
                marcoIzquierdo - (p->ancho * 0.45f); // Empujado a la derecha
            float zonaImpactoDerecha = marcoIzquierdo - (p->ancho * 0.20f);

            if ((jugadorX + 8.0f) >= zonaImpactoIzquierda &&
                (jugadorX + 8.0f) <= zonaImpactoDerecha) {
              p->estado = PUERTA_CERRADA;
              jugadorX = antiguoX; // Frenado seco
              break;
            }
          }
        }
      }
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
          // Dibujamos el túnel de fondo
          DrawRectangle(basePuerta.x - p.ancho / 2.0f, basePuerta.y - p.alto,
                        p.ancho, p.alto, BLACK);
          DrawRectangleLinesEx((Rectangle){basePuerta.x - p.ancho / 2.0f,
                                           basePuerta.y - p.alto, p.ancho,
                                           p.alto},
                               2.0f, p.color);

          // Bisagra izquierda fija
          Vector2 bisagraAbajo = {basePuerta.x - p.ancho / 2.0f, basePuerta.y};
          Vector2 bisagraArriba = {basePuerta.x - p.ancho / 2.0f,
                                   basePuerta.y - p.alto};

          // Extremo de la hoja proyectándose un 75% hacia la izquierda en
          // paralelo
          Vector2 extremoAbajo = {basePuerta.x - p.ancho / 2.0f -
                                      p.ancho * 0.75f,
                                  basePuerta.y + p.alto * 0.25f};
          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          // --- NUEVA DETECCIÓN VISUAL EN PANTALLA ---
          // Calculamos el centro geométrico real de la hoja roja en píxeles de
          // la pantalla
          float centroHojaPantallaX = (bisagraAbajo.x + extremoAbajo.x) / 2.0f;
          float centroHojaPantallaY = (bisagraAbajo.y + extremoAbajo.y) / 2.0f;

          // Calculamos la posición real de la sombra del jugador en la pantalla
          Vector2 posSueloPantalla = {jPosProyectada.x + scroll2DX,
                                      jPosProyectada.y + scroll2DY};

          // Si el jugador se mueve hacia la derecha (movX > 0) y su elipse
          // colisiona con el centro de la hoja en la pantalla, cerramos el
          // portón de inmediato
          if (movX > 0.0f) {
            float distanciaX = fabsf(posSueloPantalla.x - centroHojaPantallaX);
            float distanciaY = fabsf(posSueloPantalla.y - centroHojaPantallaY);

            // Comprobamos si la sombra (radio de 16px en X y 8px en Y) toca el
            // centro de la madera
            if (distanciaX < 20.0f && distanciaY < 12.0f) {
              mapa[salaActual].puertas[i].estado = PUERTA_CERRADA;
              jugadorX = antiguoX; // Detiene el movimiento lúdico para que no
                                   // atraviese el muro
            }
          }

          // Renderizamos la hoja con su color sólido corregido
          Color colorRellenoPuerta = {25, 25, 25, 255};
          DrawTriangle(bisagraAbajo, bisagraArriba, extremoAbajo,
                       colorRellenoPuerta);
          DrawTriangle(bisagraArriba, extremoArriba, extremoAbajo,
                       colorRellenoPuerta);

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
          Vector2 bisagraAbajo = baseFondo;
          Vector2 bisagraArriba = techoFondo;

          float anchoHojaPixeles = p.alto * 0.75f;

          Vector2 extremoAbajo;
          extremoAbajo.x = baseFondo.x + anchoHojaPixeles;
          extremoAbajo.y = baseFondo.y;

          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          // --- INVERSIÓN DE ORDEN DE VÉRTICES (Sentido antihorario correcto
          // para OpenGL) ---
          Color colorRellenoPuerta = {25, 25, 25, 255};
          DrawTriangle(bisagraAbajo, extremoAbajo, bisagraArriba,
                       colorRellenoPuerta);
          DrawTriangle(bisagraArriba, extremoAbajo, extremoArriba,
                       colorRellenoPuerta);

          // Contorno brillante idéntico a la puerta del fondo
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

          float anchoHojaPixeles = p.alto * 0.75f;

          Vector2 extremoAbajo;
          extremoAbajo.x = baseFondo.x - anchoHojaPixeles;
          extremoAbajo.y = baseFondo.y;

          Vector2 extremoArriba = {extremoAbajo.x, extremoAbajo.y - p.alto};

          // --- INVERSIÓN DE ORDEN DE VÉRTICES PARA EL LADO DERECHO ---
          Color colorRellenoPuerta = {25, 25, 25, 255};  // NHR. Cambiar aqui el color de la puerta abierta
          DrawTriangle(bisagraAbajo, bisagraArriba, extremoAbajo, colorRellenoPuerta);
          DrawTriangle(bisagraArriba, extremoArriba, extremoAbajo, colorRellenoPuerta);

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