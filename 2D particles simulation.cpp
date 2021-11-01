#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>


//do zmiany w celu dopasowania symulacji
unsigned int static_per_cell = 50;      //liczba statycznych czasteczek na komorke
unsigned int cell_size = 40;            //rozmiar komorki
float speed_change = 1;                 //wspolczynnik zmiany predkosci czasteczek >1 szybciej; <1 wolniej
float radius = 3.0;                     //promien sprawdzania czy doszlo do zderzenia

//nie przeznaczone do zmieniania
float time_simulated = 0.0;             //przebyty czas symulacji
float simulation_time;                  //przekazany docelowy czas symulacji
unsigned int number_of_collisions = 0;  //calkowita liczba zmierzonych kolizji


typedef struct static_particle{         //do przechowywania informacji o statycznych czasteczkach i o miejscach zderzenia
    float position_x;
    float position_y;
    struct static_particle *next;
    struct static_particle *prev;
} static_particle;


typedef struct moving_particle{         //dla poruszajacych sie czasteczek
    float position_x;
    float position_y;
    float velocity_x;                   //wektory przemieszczenia w danych osiach
    float velocity_y;
    struct moving_particle *next;
    struct moving_particle *prev;

} moving_particle;

moving_particle *moving_particles_positions;            //tablica przechowujaca pozycje i kierunek ruchu poruszajacych sie czasteczek


typedef struct list_m{
    struct moving_particle *head;
    struct moving_particle *tail;
} list_m;

list_m moving_particles_list = {(moving_particle *) NULL,(moving_particle *) NULL};     //lista do poruszajacych sie czasteczek


typedef struct sub_cell{
    unsigned int p;
    unsigned int q;
} sub_cell;                             //podzial plaszczyzny na mniejsze komorki

sub_cell cell = {10, 10};               //liczba komorek w obszarze, mozna dowolnie zmieniac

sub_cell *cell_indices;                 //przechowuje wspolrzedne komorek, do obliczania losowych pozycji statycznych czasteczek


typedef struct list_t {
	struct static_particle *head;
	struct static_particle *tail;
} list_t;

list_t *cells_lists;        //lista statycznych czasteczek w danej komorce
list_t collision_list = {(static_particle *) NULL,(static_particle *) NULL};    //dane o kolizjach
list_t static_particles_list = {(static_particle *) NULL,(static_particle *) NULL};     //wszystkie statyczne czasteczki




int listInit (list_t *l){       //inicjalizacja listy
    l->head = (static_particle *) NULL;
    l->tail = (static_particle *) NULL;

    return 0;
}


void collisionListput (float pos_x, float pos_y){   //dodanie kolizji do listy

    static_particle *n;
    n = (static_particle *) malloc(sizeof(static_particle));

    n->next = (static_particle *) NULL;
    n->prev = collision_list.tail;
    n->position_x = pos_x;
    n->position_y = pos_y;

    if(collision_list.tail == (static_particle *) NULL)
        collision_list.head = n;
    else
        collision_list.tail->next = n;

    collision_list.tail = n;
    number_of_collisions++;
}


void staticListput(list_t *l, float pos_x, float pos_y, sub_cell *cell_indices) {          //dodaje do listy poszczegolnej komorki
                                                                                            //czasteczke z przekazanymi wspolrzednymi
    static_particle *n;
    n = (static_particle *) malloc(sizeof(static_particle));

    n->next = (static_particle *) NULL;
    n->prev = l->tail;
    n->position_x = pos_x;
    n->position_y = pos_y;

    if(l->tail == (static_particle *) NULL)
        l->head = n;
    else
        l->tail->next = n;

    l->tail = n;

    n->next = (static_particle *) NULL;
    n->prev = static_particles_list.tail;
    n->position_x = pos_x;
    n->position_y = pos_y;

    if(static_particles_list.tail == (static_particle *) NULL)
        static_particles_list.head = n;
    else
        static_particles_list.tail->next = n;

    static_particles_list.tail = n;

}



int checkCollision(float moving_pos_x, float moving_pos_y){   //sprawdzanie czy doszlo do kolizji ze statyczna czasteczka

    static_particle *n;
    n = (static_particle *) malloc(sizeof(static_particle));

    static_particle *tmp;
    tmp = (static_particle *) malloc(sizeof(static_particle));

    for(n = static_particles_list.head; n; n = n->next){        //przejscie po wszystkich czasteczkach by sprawdzic czy doszlo do kolizji

        float d = sqrt((powf((moving_pos_x - n->position_x), 2.0) + powf((moving_pos_y - n->position_y), 2.0)));  //obliczanie odleglosci

        if(d <= radius){
            collisionListput(n->position_x, n->position_y);

            if((n->position_x == static_particles_list.head->position_x && n->position_y == static_particles_list.head->position_y) \
                && (n->position_x == static_particles_list.tail->position_x && n->position_y == static_particles_list.tail->position_y)){        //przepiecie wskaznikow, by usunac zderzona czasteczke z listy, zeby juz sie z nia nie zderzaly czasteczki

                static_particles_list.head = (static_particle *) NULL;
                static_particles_list.tail = (static_particle *) NULL;

            } else if(n->position_x == static_particles_list.head->position_x && n->position_y == static_particles_list.head->position_y){

                n->next->prev = (static_particle *) NULL;
                static_particles_list.head = n->next;

            } else if(n->position_x == static_particles_list.tail->position_x && n->position_y == static_particles_list.tail->position_y){

                n->prev->next = (static_particle *) NULL;
                static_particles_list.tail = n->prev;

            } else {
                
                tmp->next = n->next;
                tmp->prev = n->prev;
                n->prev->next = n->next;
                n->next->prev = tmp->prev;

            }

            return 0;
          }
    }
}


int checkBorder(float position, int cell_p_or_q){    //sprawdzanie, czy czasteczka znajduje sie na granicy

    if(position >= (cell_size * cell_p_or_q) || position <= 0){
        return 1;
    }

    return 0;
}


void moveSimulation(){      //zmiana polozenia poruszajacych sie czasteczek o ich wektor

    moving_particle *tmp;
    int x, y;

    tmp = (moving_particle *) malloc(sizeof(moving_particle));

    tmp = moving_particles_list.head;

    for(int i = 0; tmp; tmp = tmp->next, i++){      //petla po wszystkich ruszajacych sie czasteczkach

        tmp->position_x = (tmp->velocity_x + tmp->position_x);
        tmp->position_y = (tmp->velocity_y + tmp->position_y);
        checkCollision(tmp->position_x, tmp->position_y);
        x = checkBorder(tmp->position_x, cell.p);
        y = checkBorder(tmp->position_y, cell.q);

        if(x == 1) tmp->velocity_x *= -1;   //zmiana wektora by odbic czasteczke od sciany

        if(y == 1) tmp->velocity_y *= -1;
    }
    return;
}


int randomStaticParticle (list_t *cell_list, sub_cell *cell_indices, int integer){  //losowe przydzielanie wartosci dla statycznych czasteczek

    static_particle tmp;

    for(int i = 0; i < static_per_cell; i++){

        float rand_x = ((float)rand()/((float)(RAND_MAX))) * cell_size;
        rand_x += (cell_indices->p * cell_size);    //aby czasteczka znajdowala sie w wybranej komorce


        float rand_y = ((float)rand()/((float)(RAND_MAX))) * cell_size;
        rand_y += (cell_indices->q * cell_size);

        staticListput(cell_list, rand_x, rand_y, cell_indices); //dodawanie czasteczki do listy
    }

}


void initStaticParticles (){    //inicjalizacja statycznych czasteczek, przyznaczenie im losowych wartosci i dopisanie do listy

    int cells_number = cell.p * cell.q;
    cells_lists = malloc(cells_number * sizeof(list_t));  //po jednej liscie na kwadrat
    cell_indices = malloc(cells_number * sizeof(sub_cell));


    for(int i = 0, index = 0; i < cell.p; i++){        //przypisanie indeksow komorek
        for(int j = 0; j < cell.q; j++, index++){
            cell_indices[index].p = i;
            cell_indices[index].q = j;
        }
    }

    for(int i = 0; i < cells_number; i++){      //dla kazdej listy komorki bedzie dodawac (static_per_cell) licze czasteczek

        listInit(&(cells_lists[i]));

        randomStaticParticle(&(cells_lists[i]), &(cell_indices[i]), i); //losowe wartosci dla czasteczek
    }
}


int saveMove(char filename[]){ //zapis wlasciwosci

    moving_particle *n;
    FILE *f = NULL;

    printf("Status zapisywania pliku %s: ", filename);
    f = fopen(filename, "w");
    if((f = fopen(filename, "w")) == NULL){
    printf("NIEPOWODZENIE\n\n");
    return -1;
    }

    fprintf(f,"x_pos\ty_pos\tvel_x\tvel_y");

    for(n = moving_particles_list.head; n; n = n->next){

        fprintf(f,"\n%f\t", n->position_x);
        fprintf(f,"%f\t", n->position_y);
        fprintf(f,"%f\t", n->velocity_x);
        fprintf(f,"%f", n->velocity_y);
    }

    fclose(f);
    printf("POWODZENIE\n\n");

    return 0;
}


void movingListput (float pos_x, float pos_y, float vector_x, float vector_y){  //dodawanie poruszajacych sie czasteczek do listy

    moving_particle *n;
    n = (moving_particle *) malloc(sizeof(moving_particle));

    n->next = (moving_particle *) NULL;
    n->prev = moving_particles_list.tail;
    n->position_x = pos_x;
    n->position_y = pos_y;
    n->velocity_x = vector_x;
    n->velocity_y = vector_y;

    if(moving_particles_list.tail == (moving_particle *) NULL)
        moving_particles_list.head = n;
    else
        moving_particles_list.tail->next = n;

    moving_particles_list.tail = n;
}


void randomMovingParticle(){        //losowe dobieranie wartosci dla poruszajacych sie czasteczek

    moving_particle tmp;

    float pos_x = ((float)rand()/((float)(RAND_MAX))) * (cell.p * cell_size);

    float pos_y = ((float)rand()/((float)(RAND_MAX))) * (cell.q * cell_size);

    float vec_x = (((float)rand()/((float)(RAND_MAX)))-0.5) * speed_change;  //-0.5 zeby byly tez ujemne wektory

    float vec_y = (((float)rand()/((float)(RAND_MAX)))-0.5) * speed_change;

    movingListput(pos_x, pos_y, vec_x, vec_y);  //dodanie do listy
}


void stringAddTSV(char *string){        //dodanie '.tsv' do konca nazwy pliku

    int i = 0;
    char tsv[4] = {'.', 't', 's', 'v'};

    while(string[i] != '\0') i++;

    for(int n = 0; n < 4; n++){
        string[i + n] = tsv[n];
    }
}


void initMovingParticles(){     //inicjalizacja poruszajacych sie czasteczek

    int moving_number;
    char saving;
    int filename_length = 40;   //dlugosc stringu z nazwa pliku
    char filename[filename_length];

    for(int i = 0; i < filename_length; i++){   //czyszczenie stringu z nazwa pliku
            filename[i] = '\0';
        }

    printf("Podaj liczbe poruszajacych sie czasteczek: ");
    scanf("%d", &moving_number);
    printf("\n");

    for(int i = 0; i < moving_number; i++){
        randomMovingParticle();
    }

    printf("\n");
    printf("Czy chcesz zapisac wylosowane wlasciwosci poruszajacyh sie czasteczek?[y/n]: ");
    scanf(" %c", &saving);

    if(saving == 'y' || saving == 'Y'){
    printf("\n");
    printf("Podaj nazwe pod ktora chcesz zapisac plik: ");
    scanf("%s", filename);
    stringAddTSV(&filename[0]);
    saveMove(filename);
    }

}


int saveResult(char filename[]){  //zapis rezultatu

    static_particle *n;
    FILE *f = NULL;
    int i = 0;

    printf("Status zapisywania pliku %s: ", filename);
    f = fopen(filename, "w");
    if((f = fopen(filename, "w")) == NULL){
    printf("NIEPOWODZENIE\n\n");
    return -1;
    }

    fprintf(f,"x_pos\ty_pos");

    for(n = collision_list.head; n; n = n->next){
        fprintf(f,"\n%f\t", n->position_x);
        fprintf(f,"%f", n->position_y);
    }

    fclose(f);
    printf("POWODZENIE\n\n");

    return 0;
}


void staticListFree(list_t l) { //zwalnianie listy statycznych czasteczek

    static_particle *n;

    if (l.tail == (static_particle *) NULL) return;

    while (l.tail != (static_particle *) NULL) {

        n = l.tail;
        l.tail = l.tail->prev;
        free(n);
    }

    l.head = (static_particle *) NULL;
}


void movingListFree(list_m l) { //zwalnianie listy poruszajacych czasteczek

    moving_particle *n;

    if (l.tail == (moving_particle *) NULL) return;

    while (l.tail != (moving_particle *) NULL) {

        n = l.tail;
        l.tail = l.tail->prev;
        free(n);
    }

    l.head = (moving_particle *) NULL;
}


void myInit (void){

    glClearColor(0.0, 0.0, 0.0, 1.0);   //ustawienie tla na czarny kolor

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, (cell.p * cell_size), 0, (cell.q * cell_size));   //ustawianie wielkosci obszaru
}



void display (void) {   //dodawanie odpowiednich punktow do bufora graficznego

    glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(radius * 3);
    glEnable(GL_POINT_SMOOTH);

    glBegin(GL_POINTS);

        glColor3f(1.0, 0.0, 0.0);       //wartosci to RGB
                                        //dodawanie miejsc zderzenia do wyswietlania
        static_particle *tmp_static;
        tmp_static = (static_particle *)malloc(sizeof(static_particle));

        for(tmp_static = collision_list.head; tmp_static; tmp_static = tmp_static->next){
            glVertex2f(tmp_static->position_x, tmp_static->position_y);
        }
    glEnd();


    glPointSize(radius * 2);
    glBegin(GL_POINTS);
                                    //dodawanie statycznych punktow do wyswietlania
        glColor3f(0.0, 0.0, 0.5);

        for(tmp_static = static_particles_list.head; tmp_static; tmp_static = tmp_static->next){
            glVertex2f(tmp_static->position_x, tmp_static->position_y);
        }
    glEnd();
    
    glPointSize(radius * 2);
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POINTS);             //dodawanie poruszajacych sie czasteczek do wyswietlania

        moving_particle *tmp_moving;
        tmp_moving = (moving_particle *)malloc(sizeof(moving_particle));

        for(tmp_moving = moving_particles_list.head; tmp_moving; tmp_moving = tmp_moving->next){
            glVertex2f(tmp_moving->position_x, tmp_moving->position_y);
        }
    glEnd();

    glutSwapBuffers();  //wykorzystanie dwoch buforow, by wyswietlanie bylo plynne
}


void endProgram(){

    printf("Zakonczono dzialanie symulacji!\n\n");
    printf("Wykryto %d zderzen ze statycznymi czasteczkami\n", number_of_collisions);

    char saving;
    int filename_length = 40;   //dlugosc stringu z nazwa pliku
    char filename[filename_length]; // nazwa pliku

    for(int i = 0; i < filename_length; i++){   //czyszczenie stringu z nazwa pliku
            filename[i] = '\0';
        }

    printf("\n");
    printf("Czy chcesz zapisac uzyskane wyniki?[y/n]: ");
    scanf(" %c", &saving);

    if(saving == 'y' || saving == 'Y'){
        printf("\n");
        printf("Nazwa pliku:: ");
        scanf("%s", filename);
        stringAddTSV(&filename[0]);
        saveResult(filename);
        printf("\n");
    }
    saving == '-';

    printf("Czy chcesz zapisac koncowe wlasciwosci czasteczek?[y/n]: ");
    scanf(" %c", &saving);

    if(saving == 'y' || saving == 'Y'){
        printf("\n");
        printf("Nazwa pliku: ");
        scanf("%s", filename);
        stringAddTSV(&filename[0]);
        saveMove(filename);
    }

    exit(0);

}


void timer(){               //GLOWNA ZAPETLONA FUNKCJA, LICZACA KAZDA KLATKE

    glutPostRedisplay();    //uruchamia funkcje display i wyswietla czasteczki
    moveSimulation();       //zmiany w polozeniu i zderzenia czasteczek
    time_simulated += 1.0/60.0;     //1/60 - docelowo odswiezanie z czestotliwoscia 60Hz
    if(time_simulated > simulation_time) {      //zakonczenie symulacji po przekroczeniu czasu
        endProgram();
    }
    glutTimerFunc(1000/60, timer, 0);
}


void keyboard(unsigned char c, int x, int y){   //wychodzi z symulacji po wcisnieciu esc

    switch (c) {
        case 27:
            endProgram();
            break;
        default:
            break;
  }
}



int main(int argc, char** argv){

    char visual_on; //wybor czy wlaczac wizualizacje
    char global_options; //wybor czy zmienic opcje
    srand((unsigned int)time(NULL));
    char saving; //wybor czy zapisac rezultat
    float percent;

    

    printf("\nWitaj!\nMasz przed soba program symulujacy zderzenia czasteczek w przestrzeni dwuwymiarowej\n");
    printf("\nCzy chcesz uruchomic okno wizualizujace ruch czasteczek?\n(y/n): ");
    scanf(" %c", &visual_on);
    printf("\nCzy chcesz zmienic opcje symulacji?\n(y/n): ");
    scanf(" %c", &global_options);
    printf("\n");


    if (global_options == 'y' || global_options == 'Y'){      //Zmiana globalnych opcji symulacji
        printf("Podaj liczbe statycznych czasteczek na komorke: ");
        scanf("%d", &static_per_cell);
        printf("Podaj rozmiar komorki: ");
        scanf("%d", &cell_size);
        printf("Podaj liczbe komorek w obszarze:\n");
        printf("p: ");
        scanf("%d", &cell.p);
        printf("q: ");
        scanf("%d", &cell.q);
        printf("Podaj wspolczynnik zmiany predkosci: ");
        scanf("%f", &speed_change);
        printf("Podaj promien odciecia ponizej ktorego nastepuje zderzenie: ");
        scanf("%f", &radius);
        printf("\n");

    }


    if (visual_on == 'n' || visual_on == 'N'){      //symulacja bez wizualizacji
        printf("Podaj czas trwania symulacji: ");
        scanf("%f", &simulation_time);
        initStaticParticles();  //inicjalizacjaa symulacji
        initMovingParticles();

        while(time_simulated < simulation_time){
            moveSimulation();       //przeprowadzanie symulacji
            time_simulated += 1.0/60.0;
        }
        endProgram();
        }



    if(visual_on == 'y' || visual_on == 'Y'){       //symulacja z wizualizacja
        printf("OPIS WIZUALIZACJI:\n");
        printf("*Granatowe piksele obrazuja statyczne czasteczki*\n*Biale piksele obrazuja ruchome czasteczki*\n*Czerwone piksele obrazuja punkty w ktorych nastapilo zderzenie*\n");
        printf("*Nacisnij klawisz esc by zakonczyc symulacje*\n\n");
        printf("Podaj czas trwania symulacji: ");
        scanf("%f", &simulation_time);

        initStaticParticles();      //inicjalizacja symulacji
        initMovingParticles();
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

        int window_size_x = cell.p * cell_size * 2;     //dopasowanie rozmiaru okna
        int window_size_y = cell.q * cell_size * 2;
        if(window_size_x > 1200) window_size_x /= 2;
        if(window_size_y > 1000) window_size_x /= 2;
        glutInitWindowSize(window_size_x, window_size_y);
        glutInitWindowPosition(0, 0);                   //miejsce w ktorym okno zostanie otwarte

        glutCreateWindow("Symulacja zderzen czasteczek");
        myInit();                    //wczytanie konfiguracji wyswietlania

        glutDisplayFunc(display);
        glutTimerFunc(1000.0/60.0, timer, 0);   //wywolanie glownej petli

        glutKeyboardFunc(keyboard);     //klawisz esc konczy symulacje
        glutMainLoop();
    }

                                         //zwalnianie pamieci
    movingListFree(moving_particles_list);
    staticListFree(collision_list);
    staticListFree(static_particles_list);
    if (cell_indices != NULL) free(cell_indices);
    if (cells_lists != NULL) free(cells_lists);
    if (moving_particles_positions != NULL) free(moving_particles_positions);

    return 0;
}