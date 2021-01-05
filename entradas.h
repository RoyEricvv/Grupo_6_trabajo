#ifndef ENTRADAS_H
#define ENTRADAS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <allegro5/allegro5.h>
#include <stdint.h>

extern ALLEGRO_TIMER* timer;
extern ALLEGRO_EVENT_QUEUE* queue;

void end_cpu()
{
    delete_display();
    endwin();
}


void run_program()
{
    // end the program once the PC points to null memory
    uint8_t op_ind;
    //como hemos inicializado en 0 todo, cuando llege a 0 entonces parara el bucle
    bool done = false;
    bool redraw = true;
    ALLEGRO_EVENT event;

    al_start_timer(timer);
    while (1) {
        al_wait_for_event(queue, &event);

        if (mem.ram[cpu.pch][cpu.pcl] == 0x00) break;

        switch (event.type) {
        case ALLEGRO_EVENT_TIMER:
            mem.ram[0][0xfe] = rand() % 256;
            op_ind = fetch();
            execute(op_ind);
            info_cpu();
            break;
        case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_W)
                mem.ram[0][0xff] = 0x77;
            else if (event.keyboard.keycode == ALLEGRO_KEY_A)
                mem.ram[0][0xff] = 0x61;
            else if (event.keyboard.keycode == ALLEGRO_KEY_S)
                mem.ram[0][0xff] = 0x73;
            else if (event.keyboard.keycode == ALLEGRO_KEY_D)
                mem.ram[0][0xff] = 0x64;
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        }

        if (done) break;
    }
}





#endif
