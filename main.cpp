#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include "N6502.h"

#define LARGO 1000
#define ANCHO 720
#define LIMITE_NODOS 200

#define movingUp      1
#define movingRight   2
#define movingDown    4
#define movingLeft    8


uint8_t pcIniL = 0x0000, pcIniH = 0x0000, pcAuxL, pcAuxH;

extern ALLEGRO_TIMER* timer;
extern ALLEGRO_EVENT_QUEUE* queue;
extern ALLEGRO_DISPLAY* disp;
extern ALLEGRO_FONT* font;

ALLEGRO_TIMER* timer;
ALLEGRO_EVENT_QUEUE* queue;
ALLEGRO_DISPLAY* disp;
ALLEGRO_FONT* font;



void setUp_Pantalla()
{
    al_init();
    al_install_keyboard();
    timer = al_create_timer(1.0 / 1000.0);
    queue = al_create_event_queue();
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    disp = al_create_display(640, 650);
    al_set_window_title(disp, " EL COME ROCONES: SNAKE ");
    font = al_create_builtin_font();
    al_init_primitives_addon();


    ALLEGRO_COLOR white = al_map_rgb_f(1, 1, 1);
    ALLEGRO_COLOR red = al_map_rgb_f(1, 0, 0);
    al_draw_filled_rectangle(100, 120, 100, 120, red);

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
}



void abrir_juego(const char* nombreArch) {
    long tamanio;
    FILE* archJuego = fopen(nombreArch, "rb");
    uint16_t pcIni;
    uint8_t datos;

    /*
        fseek(archJuego,0,SEEK_END);
        tamanio = ftell(archJuego);
        fseek(archJuego,0,SEEK_SET);
        fread(&pcIniL,1,1,archJuego);
        fread(&pcIniH,1,1,archJuego);
    */
    pcAuxH = 0x06;pcAuxL = 0x00;
    char hex[] = "FF";
    while (1) {
        if (feof(archJuego)) break;
        fscanf(archJuego, "%s", hex);
        int num = (int)strtol(hex, NULL, 16);
        write6502(pcAuxL | pcAuxH << 8, num);
        if (pcAuxL == 0xff) {
            pcAuxH++;pcAuxL = 0;
        }
        else {
            pcAuxL++;
        }
    }
    fclose(archJuego);
}


void update_pantalla() {
    float x1 = 0, y1 = 0, x2 = 20, y2 = 20;
    al_clear_to_color(al_map_rgb(0, 0, 0));
    ALLEGRO_COLOR verde = al_map_rgb_f(0, 128, 0);
    ALLEGRO_COLOR rojo = al_map_rgb_f(255, 0, 0);

    for (int i = 2; i <= 5;i++)
        for (int j = 0;j < 256;j++)
            write6502(i << 8 | j, 0);
    uint8_t ramL = ram[0x0010], ramH = ram[0x0011];
    //snakeHead : guardado en $10(L) y $11(H)
    if (ramL != 0 && ramH != 0)
        ram[ramL | ramH << 8] = 0x23;

    //snakeBody (comienza en $12)
    uint8_t cuerpo = 0x12;

    while (ram[cuerpo + 1] != 0) {
        ram[ram[cuerpo] | ram[cuerpo + 1] << 8] = 0x23;
        cuerpo += 2;
    }

    //manzana (usa $00 y $01)
    //ram[0x0000] = rand()

    ramL = ram[0x0000];ramH = ram[0x0001];
    if (ramL != 0 && ramH != 0)
        ram[ramL | ramH << 8] = 0x22;
    
    //print pantalla
    for (int i = 2;i < 5;i++) {
        for (int j = 0;j < 256;j++) {
            if (ram[i << 8 | j]) {
                //printf("M: %x\n", ram[i << 8 | j]);
                if (ram[i << 8 | j] == 0x22) {
                   al_draw_filled_rectangle(x1, y1, x2, y2, rojo);
                }
                else if (ram[i << 8 | j] == 0x23) {
                   al_draw_filled_rectangle(x1, y1, x2, y2, verde);
                }
            }
            if (x2 == 640) {
                y1 += 20;
                y2 += 20;
                x1 = 0;
                x2 = 20;
            }
            else {
                x1 += 20;
                x2 += 20;
            }
        }
    }
    al_flip_display();
}

void run6502() {
    bool done = false;
    bool redraw = true;
    ALLEGRO_EVENT event;
    al_start_timer(timer);
    int cont = 0;

    while (1) {
        al_wait_for_event(queue, &event);
        opcode = read6502(pc);
        if (opcode==0x00) break;
        int flag = 0;

        switch (event.type) {
        case ALLEGRO_EVENT_TIMER:
            
            step6502();
            while (1) {
                flag = rand() % 256;
                if (ram[flag | (flag & 0x3) << 8] != 0x023) break;
            }
            ram[0x00fe] = flag;

            
            //printf("%x\n",ram[pc]); 4c
            if (ram[pc] == 0xea) {
                update_pantalla();
                //actualiza = 0;
                //flag = 1;
            }

            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_W)
                ram[0x00ff] = 0x77;
            else if (event.keyboard.keycode == ALLEGRO_KEY_A)
                ram[0x00ff] = 0x61;
            else if (event.keyboard.keycode == ALLEGRO_KEY_S)
                ram[0x00ff] = 0x73;
            else if (event.keyboard.keycode == ALLEGRO_KEY_D)
                ram[0x00ff] = 0x64;
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        }
        
        if (done) break;
    }

}


int main() {
    reset6502();
    srand(time(NULL));
    abrir_juego("snakeHexV3.txt");
    setPC(0x0600);
    setUp_Pantalla();
    run6502();

    return 0;
}




