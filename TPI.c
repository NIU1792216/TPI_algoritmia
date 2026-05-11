// 1792216 Mario Roda Sevilla
#include <float.h>
#define __USE_MINGW_ANSI_STDIO 1
#define R 6371.0 // Radi de la Terra en km
#define PI 3.1415926535897932384626433832795028841971
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
// Definim l'estructura d'un element de la cua, que apunta al seguent
typedef struct ElementCua{
    Node *node;
    struct ElementCua *seguent;
} ElementCua;
// Definim l'estructura de la cua, on guardem el primer i l'ultim element
typedef struct {
    ElementCua *inici;
    ElementCua *final;
} Cua;

// Definim una funcio per calcular la distancia real entre dos nodes
double distancia(const Node *node1, const Node *node2);
// Definim la funcio per calcular el cami recorregut fins arribar a un node
double pes(Node *n);
// Definim una funcio per trobar la posicio d'un node a la llista de nodes a partir del seu id
long unsigned index_node(unsigned long long ident, const Node *nodes, unsigned long num_nodes);
// Definim una funcio per llegir un fitxer amb nodes
void llegir_nodes(char nom_fitxer[], Node **nodes, unsigned long *num_nodes_llegits);
// Definim una funcio per llegir un fitxer amb carrers i guardar les dades en uns nodes donats
void llegir_carrers(char nom_fitxer[], Node *nodes, unsigned long num_nodes);
// Definim una funcio per afegir a dos nodes la conexio entre cada un d'ells a traves d'un carrer
void afegir_conexio(Node *node1, Node *node2, unsigned long id_carrer);
// Definim la funcio per posar elements a la cua
void encua(Cua *cua, Node *nodes, unsigned long num_nodes, Node *n, Node*objectiu, const double *pesos);
// Definim la funcio per canviar la posicio a la cua d'un element
void reencua(Cua *cua, Node *nodes, unsigned long num_nodes, Node *n, Node*objectiu, const double *pesos);
// Definim la funcio per treure un node de la cua
Node *desencua(Cua *c);
// Definim una funcio per poder ordenar llistes de nodes pel seu id
int comparar_nodes(const void *a, const void *b);
// Definim una funcio per mostrar el cami recorregut fins a un node donat
void mostrar_cami(Node *n);
// Definim una funcio per guardar el cami en un fitxer .txt
void guardar_cami(Node *n, char nom_fitxer[]);
// Definim la funcio per recorrer una llista i trobar el cami mes eficient (A*)
bool A(Node *, unsigned long num_nodes, Node *inicial, Node*final);

int main(int argc, char *argv[]){
    // Comprovem que hi hagi dos arguments en la crida del programa
    if (argc != 3){
        printf("S'han d'introduir 2 nodes");
        exit(EXIT_FAILURE);
    }
    char *error1, *error2;
    // id del node inicial
    unsigned long long id_inicial = strtoull(argv[1], &error1, 10);
    // id del node final
    unsigned long long id_final = strtoull(argv[2], &error2, 10);
    // Comprovem si hi ha hagut cap error en la lectura dels arguments del programa
    if (id_inicial == 0 && (error1 == argv[1] || error2 == argv[2])){
        printf("Els id introduits han de ser valors enters en base decimal\n");
        if (error1 == argv[1]){
            printf("El programa no ha pogut llegir a partir del caracter %s\n\n", error1);
        } else{
            printf("El programa no ha pogut llegir a partir del caracter %s\n\n", error2);
        }
        exit(EXIT_FAILURE);
    }
    // Llista amb tots els nodes
    Node *nodes=NULL;
    // Nombre de nodes
    unsigned long num_nodes;
    // Llegim les dades del fitxer Nodes.csv i guardem els nodes en una llista
    llegir_nodes("Nodes_Sb.csv", &nodes, &num_nodes);
    printf("S'han llegit %lu nodes del fitxer Nodes.csv\n", num_nodes);
    // Ordenem la llista de nodes per poder utilitzar la cerca binaria
    qsort(nodes, num_nodes, sizeof(Node), comparar_nodes);
    // Llegim les dades del fitxer Carrers.csv i guardem la informacio a nodes
    llegir_carrers("Carrers_Sb.csv", nodes, num_nodes);
    printf("S'ha llegit correctament el fitxer amb carrers\n");
    
    // Localitzem el node inicial
    Node *inicial = &nodes[index_node(id_inicial, nodes, num_nodes)];
    // Localitzem el node final
    Node *final = &nodes[index_node(id_final, nodes, num_nodes)];
    bool trobat = A(nodes, num_nodes, inicial, final);

    if (!trobat) {
        printf("No s'ha pogut trobar un cami des del node %llu fins al node %llu\n", id_inicial, id_final);
    } else {
        mostrar_cami(final);
        guardar_cami(final, "out.txt");
    }
    // Alliberem memoria
    for (unsigned long i=0; i<num_nodes; i++) free(nodes[i].conexions);
    free(nodes);
    return 0;
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
double pes(Node *n){
    if (n->pare == NULL){
        return 0;
    }
    double pes = distancia(n, n->pare);
    Node *actual = n;
    while (actual->pare != NULL){
        pes += distancia(actual, actual->pare);
        actual = actual->pare;
    }
    return pes;
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
void encua(Cua *cua, Node *nodes, unsigned long num_nodes, Node *n, Node*objectiu, const double *pesos){
    unsigned long index_n = index_node(n->id, nodes, num_nodes);
    ElementCua *e;
    if ((e = (ElementCua *)malloc(sizeof(ElementCua)))==NULL){
        perror("ERROR: No s'ha pogut reservar memoria");
        exit(EXIT_FAILURE);
    }
    e->seguent = NULL;
    e->node = n;
    if (cua->inici == NULL){
        cua->inici = e;
        cua->final = e;
        return;
    }
    unsigned long index_comparativa = index_node((cua->inici)->node->id, nodes, num_nodes);
    if ((pesos[index_n] + distancia(n, objectiu)) < (pesos[index_comparativa] + distancia(&nodes[index_comparativa], objectiu))){
        e->seguent = cua->inici;
        cua->inici = e;
        return;
    }
    ElementCua *actual = cua->inici;
    bool colocat = false;
    while (actual->seguent != NULL && !colocat){
        index_comparativa = index_node((actual->seguent)->node->id, nodes, num_nodes);
        if ((pesos[index_n] + distancia(n, objectiu)) < (pesos[index_comparativa] + distancia(&nodes[index_comparativa], objectiu))){
            e->seguent = actual->seguent;
            actual->seguent = e;
            colocat = true;
        }
        else {actual = actual->seguent;}
    }
    if (actual->seguent == NULL){
        actual->seguent = e;
        cua->final = e;
    }
}
void reencua(Cua *cua, Node *nodes, unsigned long num_nodes, Node *n, Node *objectiu, const double *pesos){
    if (cua->inici == NULL){
        encua(cua, nodes, num_nodes, n, objectiu, pesos);
        return;
    }
    ElementCua *actual = cua->inici;
    if (actual->node == n) {
        cua->inici = actual->seguent;
        if (cua->inici == NULL) {
            cua->final = NULL;
        }
        free(actual);
        encua(cua, nodes, num_nodes, n, objectiu, pesos);
        return;
    }
    while (actual->seguent != NULL){
        if ((actual->seguent)->node == n){
            // Element que buscabem
            ElementCua *e = actual->seguent;
            // Si volem treure l'ultim de la cua, hem de modificar l'apuntador al final
            if (e->seguent == NULL) cua->final = actual;
            // Fem que la cua perdi referencia a l'element buscat
            actual->seguent = e->seguent;
            free(e);
            encua(cua, nodes, num_nodes, n, objectiu, pesos);
            return;
        }
        actual = actual->seguent;
    }
    printf("El node no estava a la cua");
    exit(EXIT_FAILURE);
}
Node *desencua(Cua * cua){
    if (cua->inici == NULL){
        return NULL;
    }
    ElementCua *primer_element = cua->inici;
    Node * node_desencuat = primer_element->node;
    cua->inici = primer_element->seguent;
    free(primer_element);
    if (cua->inici ==  NULL){
        cua->final = NULL;
    }
    return node_desencuat;
}
int comparar_nodes(const void *a, const void *b) {
    const Node *node_a = a;
    const Node *node_b = b;
    if(node_a->id < node_b->id) {
        return -1;
    }
    return 1;
}
void mostrar_cami(Node *n){
    // Comptador de nodes al cami
    unsigned long pasos=0;
    // Node per recorrer tot el cami
    Node *actual = n;
    // Llista amb les direccions dels nodes del cami
    Node **recorregut = NULL;
    while (actual != NULL){
        pasos++;
        if ((recorregut = (Node **)realloc(recorregut, sizeof(Node *) * (pasos))) == NULL){
            perror("ERROR: No s'ha pogut reservar memoria per al vector de nodes");
            exit(EXIT_FAILURE);
        }
        recorregut[pasos - 1] = actual;
        actual = actual->pare;
    }
    for (unsigned i=0; i<pasos; i++) {
        Node *pas = recorregut[pasos - 1 - i];
        printf("Id=%12llu | %7.6g | %7.6g | Dist=%.6g\n", pas->id, pas->latitud, pas->longitud, pes(pas));
    }
    free((void *)recorregut);
}
void guardar_cami(Node *n, char nom_fitxer[]){
    // Comptador de nodes al cami
    unsigned long pasos=0;
    // Node per recorrer tot el cami
    Node *actual = n;
    // Llista amb les direccions dels nodes del cami
    Node **recorregut = NULL;
    while (actual != NULL){
        pasos++;
        if ((recorregut = (Node **)realloc(recorregut, sizeof(Node *) * (pasos))) == NULL){
            perror("ERROR: No s'ha pogut reservar memoria per al vector de nodes");
            exit(EXIT_FAILURE);
        }
        recorregut[pasos - 1] = actual;
        actual = actual->pare;
    }
    FILE *fitxer=NULL;
    fitxer = fopen(nom_fitxer,"w");
    if (fitxer == NULL){
        printf("No s'ha pogut obrir el fitxer %s\n", nom_fitxer);
        exit(EXIT_FAILURE);
    }
    for (unsigned i=0; i<pasos; i++) {
        Node *pas = recorregut[pasos - 1 - i];
        fprintf(fitxer, "Id=%12llu | %7.6g | %7.6g | Dist=%.6g\n", pas->id, pas->latitud, pas->longitud, pes(pas)); //NOLINT
    }
    fclose(fitxer);
    free((void *)recorregut);

}
bool A(Node *nodes, unsigned long num_nodes, Node *inicial, Node *final){
        // La cua que utilitzarem
    Cua cua={NULL, NULL};
    // Marcador dels nodes visitats
    bool *encuat = NULL;
    // Llista de pesos
    double *pesos = NULL;

    if ((encuat = (bool *)calloc(num_nodes, sizeof(bool))) == NULL){
        perror("ERROR: No s'ha pogut reservar memoria per al vector de nodes");
        exit(EXIT_FAILURE);
    }
    if ((pesos = (double *)malloc(num_nodes * sizeof(double))) == NULL){
        perror("ERROR: No s'ha pogut reservar memoria per al vector de nodes");
        exit(EXIT_FAILURE);
    }
    // Inicialitzem tots els pesos al valor maxim posible del tipus de variable en la que emmagatzenem els pesos
    for (unsigned long i=0; i<num_nodes; i++) pesos[i] = DBL_MAX; 
    unsigned long index_inicial = index_node(inicial->id, nodes, num_nodes);
    // Posem el pes del node inicial a 0
    pesos[index_inicial] = 0;
    // Guardem la direccio del node inicial
    encua(&cua, nodes, num_nodes, inicial, final, pesos);
    // Indicador si hem trobat el cami
    bool trobat = false;
    while (cua.inici != NULL){
        Node *actual = desencua(&cua);
        // Si el trobem ho marquem i surtim de la cerca
        if (actual == final) {
            trobat = true;
            break;
        }
        unsigned long index_actual = index_node(actual->id, nodes, num_nodes);
        encuat[index_actual] = false;
        // Iterem sobre tots els fills
        for (unsigned i=0; i<actual->num_conexions; i++){
            unsigned long long id_fill = actual->conexions[i].id_node;
            unsigned long index_fill = index_node(id_fill, nodes, num_nodes);
            Node * fill = &nodes[index_fill];
            double nou_pes = pesos[index_actual] + distancia(actual, fill);
            if (nou_pes < pesos[index_fill]){
                fill->pare = actual;
                pesos[index_fill] = nou_pes;
                if (encuat[index_fill]) {
                    reencua(&cua, nodes, num_nodes, fill, final, pesos);
                }
                else{
                    encua(&cua, nodes, num_nodes, fill, final, pesos);
                    encuat[index_fill] = true;
                }
            }
        }
    }
    free(encuat);
    free(pesos);
    ElementCua *e_actual = cua.inici;
    ElementCua *e_anterior=NULL;
    while(e_actual != NULL){
        e_anterior = e_actual;
        e_actual = e_actual->seguent;
        free(e_anterior);
    }
    free(e_actual);
    // Retornem un bool amb si hem trobat el cami o no 
    return trobat;
}