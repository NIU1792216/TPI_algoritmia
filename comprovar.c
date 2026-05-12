#include <float.h>
#define __USE_MINGW_ANSI_STDIO 1
#define R 6371.0 // Radi de la Terra en km
#define PI 3.1415926535897932384626433832795028841971
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Definim l'estructura per conectar nodes
typedef struct {
    unsigned id_carrer;
    unsigned long long id_node;
    double longitud;
} Conexio;
// Definim l'estructura de node
typedef struct Node{
    unsigned long long id;
    double latitud, longitud;
    unsigned num_conexions;
    Conexio *conexions;
    struct Node * pare;
} Node;

// Definim una funcio per calcular la distancia real entre dos nodes
double distancia(const Node *node1, const Node *node2);
// Definim una funcio per llegir un fitxer amb nodes
void llegir_nodes(char nom_fitxer[], Node **nodes, unsigned long *num_nodes_llegits);
// Definim una funcio per llegir un fitxer amb carrers i guardar les dades en uns nodes donats
void llegir_carrers(char nom_fitxer[], Node *nodes, unsigned long num_nodes);
// Definim una funcio per trobar la posicio d'un node a la llista de nodes a partir del seu id
long unsigned index_node(unsigned long long ident, const Node *nodes, unsigned long num_nodes);
// Definim una funcio per poder ordenar llistes de nodes pel seu id
int comparar_nodes(const void *a, const void *b);
void afegir_conexio(Node *node1, Node *node2, unsigned long id_carrer);

int main(){
    // Llista amb tots els nodes
    Node *nodes=NULL;
    // Nombre de nodes
    unsigned long num_nodes=0;
    // Llegim les dades del fitxer Nodes.csv i guardem els nodes en una llista
    llegir_nodes("Nodes_And.csv", &nodes, &num_nodes);
    printf("S'han llegit %lu nodes del fitxer Nodes.csv\n", num_nodes);
    // Ordenem la llista de nodes per poder utilitzar la cerca binaria
    qsort(nodes, num_nodes, sizeof(Node), comparar_nodes);
    // Llegim les dades del fitxer Carrers.csv i guardem la informacio a nodes
    llegir_carrers("Carrers_And.csv", nodes, num_nodes);
    printf("S'ha llegit correctament el fitxer amb carrers\n");
    // Numero nodes conexos
    unsigned long num_conexos=0;
    // Numero nodes inconexos
    unsigned long num_inconexos=0;
    // fitxer conexos
    FILE *fitxer=NULL;
    // fitxer inconexos
    FILE *fitxer2=NULL;
    fitxer = fopen("inconexos_And.csv", "w");
    fitxer2 = fopen("conexos_And.csv", "w");
    for (unsigned long i=0; i<num_nodes; i++){
        if (nodes[i].num_conexions == 0){
            num_inconexos++;
            fprintf(fitxer, "%llu\n", nodes[i].id); //NOLINT
        } else{
            num_conexos++;
            fprintf(fitxer2, "%llu\n", nodes[i].id); //NOLINT
        }
    }
    printf("Hi ha %lu nodes conexos\n", num_conexos);
    printf("Hi ha %lu nodes inconexos\n", num_inconexos);
    fclose(fitxer); //NOLINT
    fclose(fitxer2); //NOLINT
    return 0;
}

void llegir_nodes(char nom_fitxer[], Node **nodes, unsigned long *num_nodes_llegits){
    /*
    El format de fitxer que es llegeix per aquesta funcio es el seguent:
        - Un fitxer ordenat on en cada linia hi ha les dades d'un node
        - Cada linia te el format id;latitud;longitud
        - No hi ha linies buides
    */
    // Comptador dels nodes del fitxer (correspon tambe a les linies del fitxer)
    unsigned long num_nodes = 0;
    FILE *fitxer=NULL;
    fitxer = fopen(nom_fitxer, "r");
    if(fitxer == NULL) {
        perror("No s'ha pogut obrir el fitxer Nodes.csv\n");
        exit(EXIT_FAILURE);
    }
    // Contador del nombre de nodes del fitxer
    char caracter;
    while((caracter = fgetc(fitxer)) != EOF) {
        if (caracter == '\n') {
            num_nodes++;
        }
    }
    // Reservem memoria per al vector de nodes
    if((*nodes = (Node*)malloc(num_nodes * sizeof(Node))) == NULL) { //NOLINT
        perror("ERROR: No s'ha pogut reservar memoria per al vector de nodes");
        exit(EXIT_FAILURE);
    }
    // Tornem a l'inici del fitxer per llegir les dades dels nodes i guardar-les al vector
    rewind(fitxer);
    for(unsigned i = 0; i < num_nodes; i++) {
        fscanf(fitxer, "%llu;%lf;%lf\n", &(*nodes)[i].id, &(*nodes)[i].latitud, &(*nodes)[i].longitud); //NOLINT
        (*nodes)[i].num_conexions = 0;
        (*nodes)[i].conexions = NULL;
        (*nodes)[i].pare = NULL;
    }
    fclose(fitxer);
    
    // Guardem el nombre de nodes llegits a la direccio de l'argument
    *num_nodes_llegits = num_nodes;
}
void llegir_carrers(char nom_fitxer[], Node *nodes, unsigned long num_nodes){
    /*
    El format de fitxer que llegeix aquesta funcio es el seguent:
        - Un fitxer ordenat on en cada linia hi ha la informacio d'un carrer
        - Cada linia te el format id_carrer;node_1;node_2;...;node_n
        - No hi ha linies buides
        - Tots els nodes que apareixen han d'estar en la llista de nodes pasada com a argument
    
    Per llegir-lo primer contarem el nombre de linies i quants nodes te cada linia pel nombre dels caracters "\n" i ";".
    Despres ja llegirem el fitxer linia a linia modificant els nodes de la llista de nodes donada com a argument
    per afegir-los les conexions de cada node
    */
    // --------------- Lectura informacio basica carrers ---------------------
    FILE *fitxer=NULL;
    fitxer = fopen(nom_fitxer,"r");
    if (fitxer == NULL){
        perror("No s'ha pogut obrir el fitxer Carrers.csv\n");
        exit(EXIT_FAILURE);
    }
    // En el vector num_conexions guardarem el nombre de nodes que conte cada carrer
    // on l'element i-essim del vector contindra el nombre de nodes que hi ha a la linia i+1 del fitxer
    unsigned *num_conexions;
    // Reservem memoria per les dades del primer carrer 
    // i inicialitzem a 0 per poder augmentar l'element directament
    if ((num_conexions = (unsigned *)calloc(1, sizeof(unsigned))) == NULL) {
        perror("ERROR: No s'ha pogut reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    // En una variable guardem el nombre de carrers que conte el fitxer (correspon al nombre de linies) 
    unsigned long num_carrers = 0; 
    // Comptem el nombre de carrers del fitxer i el nombre de nodes de cada un
    int caracter=0;
    while((caracter = fgetc(fitxer)) != EOF) {
        if (caracter == ';'){
            num_conexions[num_carrers]++;
        }
        if (caracter == '\n') {
            num_carrers++;
            // Per cada linia llegida, reservem memoria per guardar les dades d'una possible seguent linia
            if((num_conexions = (unsigned *)realloc(num_conexions, (num_carrers+1)*sizeof(unsigned))) == NULL) {
                perror("ERROR: No s'ha pogut reservar memoria");
                exit(EXIT_FAILURE);
            }
            num_conexions[num_carrers] = 0;
        }
    }
    // Deixem de reservar la memoria reservada per la linia seguent a la ultima, que en realitat no existeix
    if((num_conexions = (unsigned *)realloc(num_conexions, (num_carrers)*sizeof(unsigned))) == NULL) { //NOLINT
        perror("ERROR: No s'ha pogut reservar memoria");
        exit(EXIT_FAILURE);
    }
    rewind(fitxer);
    // --------------- Lectura de conexions de cada carrer ------------------
    for (long unsigned i=0; i<num_carrers; i++){
        // en conexions guardarem l'index dels nodes del carrer que analitzem
        Node ** conexions=NULL;
        if((conexions = (Node **)malloc(num_conexions[i]*sizeof(Node *))) == NULL) { //NOLINT
            perror("ERROR: No s'ha pogut reservar memoria");
            exit(EXIT_FAILURE);
        }
        // Llegim la primera columna de la linia que correspon amb la id del carrer
        unsigned long id_carrer=0;
        fscanf(fitxer, "id=%lu", &id_carrer); //NOLINT
        // printf("Carrer %lu, amb %u conexions", id_carrer, num_conexions[i]);
        // Despres llegim tantes vegades com nodes hi ha al carrer
        for (unsigned j=0; j<num_conexions[i]; j++){
            unsigned long long id_node=0;
            fscanf(fitxer, ";%llu", &id_node); //NOLINT
            conexions[j] = &nodes[index_node(id_node, nodes, num_nodes)];
            if (j == 0) {continue;}
            afegir_conexio(conexions[j], conexions[j-1], id_carrer);
        }
        // Descartem el salt de linia
        (void)fgetc(fitxer); //NOLINT
        free((void *)conexions);
    }
    fclose(fitxer);
    free(num_conexions);
}
long unsigned index_node(unsigned long long ident, const Node *nodes, unsigned long num_nodes){
    // Asumim que la llista nodes esta ordenada
    if (num_nodes == 0) {
        printf("La llista es buida");
        exit(EXIT_FAILURE);
    }
    // Trobem l'index amb una cerca binaria
    unsigned long a = 0;
    unsigned long b = num_nodes - 1;
    while(a <= b) {
        unsigned long m = (a + b) / 2;
        if(nodes[m].id == ident) {
            return m;
        }
        if(nodes[m].id < ident) {
            a = m + 1;
        } else {
            b = m - 1;
        }
    }
    // Si no s'ha trobat cap node amb l'identificador donat, imprimim un missatge d'error i sortim del programa
    printf("No s'ha trobat cap node amb l'identificador %llu\n", ident);
    exit(EXIT_FAILURE);
}
int comparar_nodes(const void *a, const void *b) {
    const Node *node_a = a;
    const Node *node_b = b;
    if(node_a->id < node_b->id) {
        return -1;
    }
    return 1;
}
double distancia(const Node *node1, const Node *node2){
    // Convertim les latituds de graus a radians
    double lat1_rad = node1->latitud * PI / 180.0;
    double lat2_rad = node2->latitud * PI / 180.0;
    // Convertim les longituds de graus a radians
    double long1_rad = node1->longitud * PI / 180.0;
    double long2_rad = node2->longitud * PI / 180.0;
    // Calculem les diferències de latitud i longitud en radians
    double dlat = lat2_rad - lat1_rad;
    double dlong = long2_rad - long1_rad;
    //Apliquem la fórmula de Haversine: 
    double a = pow(sin(dlat / 2), 2) + cos(lat1_rad) * cos(lat2_rad) * pow(sin(dlong / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distancia = R * c;
    return distancia;
}
void afegir_conexio(Node *node1, Node *node2, unsigned long id_carrer){
    node1->num_conexions++;
    node2->num_conexions++;
    // Afegim la conexio node1-node2 a la instancia del node 1
    if ((node1->conexions = (Conexio *)realloc(node1->conexions, node1->num_conexions*sizeof(Conexio)))==NULL){
        printf("ERROR: No s'ha pogut reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    Conexio conexio12 = {id_carrer, node2->id, distancia(node1, node2)};
    node1->conexions[node1->num_conexions - 1] = conexio12;
    // Afegim la conexio node1-node2 a la instancia del node 2
    if ((node2->conexions = (Conexio *)realloc(node2->conexions, node2->num_conexions*sizeof(Conexio)))==NULL){
        printf("ERROR: No s'ha pogut reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    Conexio conexio21 = {id_carrer, node1->id, distancia(node2, node1)};
    node2->conexions[node2->num_conexions - 1] = conexio21;
}
