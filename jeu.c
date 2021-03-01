/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>

// Paramètres du jeu
#define LARGEUR_MAX 42 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

#define LIGNES 6
#define COLONNES 7

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))


// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {
	int joueur; // à qui de jouer ? 	0 si humain, 1 pour ordinateur
	int plateau[LIGNES][COLONNES];	//-1 case vide, 0 jeton humain, 1 jeton ordinateur	
} Etat;

// Definition du type Coup
typedef struct {
	int colonne;
	int ligne;
} Coup;

// Copier un état 
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;
	
	int i,j;	
	for (i = 0; i < LIGNES; i++)
		for ( j = 0; j <COLONNES; j++)
			etat->plateau[i][j] = src->plateau[i][j];
	

	
	return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));
	
	int i,j;	
	for (i=0; i < LIGNES; i++)
		for ( j=0; j < COLONNES; j++)
			etat->plateau[i][j] = -1;
	
	return etat;
}

void draw_ligne(SDL_Renderer* pRenderer,int x, int y, int w){
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = 1;
	
	SDL_RenderFillRect(pRenderer,&rect);
}

void draw_circle(SDL_Renderer* pRenderer, int cx, int cy, int r, int v, int b, int rayon){
	int d = 3 - (2*rayon);
	int x = 0;
	int y = rayon;
	
	SDL_SetRenderDrawColor(pRenderer,r,v,b,255);
	
	while(y >= x){
		draw_ligne(pRenderer,cx-x,cy-y,2*x+1);
		draw_ligne(pRenderer,cx-x,cy+y,2*x+1);
		draw_ligne(pRenderer,cx-y,cy-x,2*y+1);
		draw_ligne(pRenderer,cx-y,cy+x,2*y+1);
		
		if(d < 0)	d = d + (4*x)+6;
		else{
			d = d + 4 * (x-y) + 10;
			y--;
		}
		x++;
	}
}

void jeton(SDL_Renderer* pRenderer, int x, int y, int r, int v, int b){
	
	draw_circle(pRenderer,x,y,255,255,255,30);
	draw_circle(pRenderer,x,y,r,v,b,25);
}

void afficheJeu_sdl(SDL_Window* fenetre, SDL_Renderer* pRenderer, Etat * etat) {

	//Initalisation de la fenêtre graphique

	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"[DEBUG] > %s",SDL_GetError());
		return;
	}
	
	if(SDL_CreateWindowAndRenderer(600,600,SDL_WINDOW_SHOWN,&fenetre,&pRenderer) < 0){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"[DEBUG] > %s",SDL_GetError());
		return;
	}
	
	//Affichage Puissance 4
	
	//Suppression affichage actuel
	SDL_SetRenderDrawColor(pRenderer,0,0,0,255);
	SDL_RenderClear(pRenderer);
	
	//Dessin jeu
	SDL_Rect fond;
	fond.x = 20;	fond.y = 100;
	fond.w = 560;	fond.h = 480;
	
	SDL_SetRenderDrawColor(pRenderer,27,1,155,255);
	SDL_RenderFillRect(pRenderer, &fond);
	
	int i,j;
	int x = 60;
	int y =140;
	for (i = 0; i < LIGNES; i++){
		for ( j = 0; j <COLONNES; j++){
			if(etat->plateau[i][j] == 1)	jeton(pRenderer,x,y,187,11,11);	//Jeton rouge pour l'humain
			if(etat->plateau[i][j] == 0)	jeton(pRenderer,x,y,218,179,10);	//Jeton jaune pour l'ordinateur
			if(etat->plateau[i][j] == -1)	jeton(pRenderer,x,y,0,0,0);		//Jeton noir si pas de jeton
			x = x + 80;
		}
		x = 60;
		y = y + 80;
	}
	
	//Mise à jour fenêtre
	SDL_RenderPresent(pRenderer);
	
	/*//Fermeture fenêtre si clic sur X
	
	SDL_Event events;
	int isOpen = 1;
	while(isOpen){
		while(SDL_PollEvent(&events)){
			switch(events.type)
			{
				case SDL_QUIT:
					SDL_Quit();
					break;
			}
		}
	}
	*/
}

void afficheJeu_terminal(Etat * etat){

	int i,j;
	printf("   |");
	for ( j = 0; j < COLONNES; j++) 
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");
	
	for(i=0; i < LIGNES; i++) {
		printf(" %d |", i);
		for ( j = 0; j < COLONNES; j++) 
			printf(" %d |", etat->plateau[i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}

}

// Nouveau coup
Coup * nouveauCoup( Etat * etat, int j ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));
	
	coup->colonne = j;
	int i;
	for (i = 0; i < LIGNES; i++){
		if (etat->plateau[i][j] == -1)
			coup->ligne = i;
	}
	
	return coup;
}

// Demander à l'humain quel coup jouer 
Coup * demanderCoup (Etat * etat) {

	int j = -1, x = -1, y;
	// 560 est la largeur de l'écran
	int width = 560;
	int intervalle = width / COLONNES;
	
	SDL_Event events;
	if(SDL_PollEvent(&events)){
		if (events.type == SDL_MOUSEBUTTONDOWN){
			// Récupère la position du curseur
			SDL_GetMouseState(&x, &y);
			j = floor(x / intervalle);
		}
	}

	return nouveauCoup(etat, j);
}

//Vérifie si dans l'état etat, il y a des jetons en dessous du jeton positionné en i,j
//Retourne 1 si oui, 0 sinon
int jetons_dessous(Etat *etat, int i, int j){
	i = i + 1;
	while(i < LIGNES){
		if(etat->plateau[i][j] == -1)	return 0;
		i = i + 1;
	}
	return 1;
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	//Si le coup est en-dehors de la grille
	if((coup->ligne >= LIGNES) || (coup->colonne >= COLONNES))	return 0;
	else{
		//S'il y a déjà un pion à cette position
		if ( etat->plateau[coup->ligne][coup->colonne] != -1 )
			return 0;
		else {
			// Ceci n'est plus nécessaire car on met le jeton dans la première ligne non occupée
			/*//S'il n'y a pas de jeton en dessous pour le soutenir
			if(jetons_dessous(etat,coup->ligne,coup->colonne)){
				//todo effacer : 
				printf("jeton dessous\n");
				return 0;
			}*/
			// Si la colonne n'est pas définie par le joueur
			if (coup->colonne == -1) return 0;
			else{
				// Si le joueur est humain, alors on met 1 dans le plateau, sinon 0.
				etat->plateau[coup->ligne][coup->colonne] = etat->joueur ? 1 : 0;
				
				// à l'autre joueur de jouer
				etat->joueur = AUTRE_JOUEUR(etat->joueur); 	

				return 1;
			}
		}
	}	
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {
	
	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );
	
	int k = 0;
	
	int i,j;
	for(i=0; i < LIGNES; i++) {
		for (j=0; j < COLONNES; j++) {
			if ( etat->plateau[i][j] == -1 ) {		//Si case vide
				if(jetons_dessous(etat,i,j)){		//S'il y a des jetons en-dessous
					coups[k] = nouveauCoup(etat, j); 
					k++;
				}
			}
		}
	}
	
	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud 
typedef struct NoeudSt {
		
	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici
	
	Etat * etat; // etat du jeu
			
	struct NoeudSt * parent; 
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste
	
	// POUR MCTS:
	int nb_victoires;
	int nb_simus;
	
} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));
	
	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;			
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);		
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0; 
	}
	noeud->parent = parent; 
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;	
	

	return noeud; 	
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);
		
	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup); 

	free(noeud);
}
	

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {
	
	// tester si un joueur a gagné
	int i,j,k,n = 0;
	int diagonales = min(LIGNES, COLONNES);
	for ( i=0;i < LIGNES; i++) {
		for(j=0; j < COLONNES; j++) {
			if ( etat->plateau[i][j] != -1) {
				n++;	// nb coups joués
			
				// lignes
				k=0;
				while ( k < LIGNES && i+k < LIGNES && etat->plateau[i+k][j] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 1? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < COLONNES && j+k < COLONNES && etat->plateau[i][j+k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 1? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < diagonales && i+k < diagonales && j+k < diagonales && etat->plateau[i+k][j+k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 1? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < diagonales && i+k < diagonales && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[i][j] == 1? ORDI_GAGNE : HUMAIN_GAGNE;		
			}
		}
	}

	// et sinon tester le match nul	
	if ( n == LIGNES * COLONNES ) 
		return MATCHNUL;
		
	return NON;
}

double calculB(Noeud * noeud){
	if(noeud->nb_simus == 0)	return DBL_MAX;
	double moy = (double) noeud->nb_victoires/noeud->nb_simus;
	if(noeud->parent->joueur == 1){
		moy = moy * -1;
	}
	return moy + sqrt(2)*sqrt(log(noeud->parent->nb_simus)/noeud->nb_simus);
}


//Renvoie le prochain noeud selon le principe UTC
Noeud * UTC(Noeud * racine){
	Noeud * current = racine;
	
	if((testFin(current->etat) != NON)||(current->nb_enfants != sizeof(coups_possibles(current->etat))/sizeof(Coup *)))	return current;
	
	Noeud * next = current->enfants[0];
	double val = calculB(next);
	double temp;
	for(int i = 1; i<current->nb_enfants; i++){
		temp = calculB(current->enfants[i]);
		if(val < temp){
			next = current->enfants[i];
			val = temp;
		}
	}
	
	return UTC(next);
}

Noeud * expansion(Noeud * current){

	if(testFin(current->etat) != NON)	return current;
	
	Coup ** coups = coups_possibles(current->etat);
	
	int k = 0;
	int i,j;
	int exist = 0;
	while(coups[k] != NULL){
		exist = 0;
		for(i=0; i < current->nb_enfants; i++){
			if(coups[k]->colonne == current->enfants[i]->coup->colonne){
				exist = 1;
				break;
			}
		}
		if(exist){
			free(coups[k]);
			j = k;
			while(coups[j] != NULL){
				coups[j] = coups[j+1];
				j++;
			}
		}
		else{
			k++;
		}
	}
	int fils = rand()%k;
	k = 0;
	while(coups[k] != NULL){
		if(k != fils){
			free(coups[k]);
		}
		k++;
	}
	
	return ajouterEnfant(current,coups[fils]);
}

FinDePartie simulation(Etat * etat){
	FinDePartie result;
	Coup ** coups;
	Coup * next_coup;
	int k;
	Etat * test;
	
	while((result = testFin(etat)) == NON){
		coups = coups_possibles(etat);
		next_coup = NULL;
		k = 0;
		while(coups[k] != NULL && next_coup == NULL){
			test = copieEtat(etat);
			jouerCoup(test,coups[k]);
			if(testFin(test) == ORDI_GAGNE){
				next_coup = coups[k];
			}
			k++;
		}
		if(next_coup == NULL){
			next_coup = coups[rand()%k];
		}
	}
	jouerCoup(etat,next_coup);
	return result;
}

void propa_result(Noeud * noeud, FinDePartie result){
	while(noeud != NULL){
		noeud->nb_simus++;
		if(result == ORDI_GAGNE){
			noeud->nb_victoires++;
			noeud->nb_victoires++;
		}
		noeud = noeud->parent;
	}
}

Noeud * meilleurFils(Noeud * racine){
	Noeud * meilleur = racine->enfants[0];
	int maxSimus;
	double maxVal, currentVal;
	
	if(meilleur->nb_simus == 0)	maxVal = 0;
	else				maxVal = (double) meilleur->nb_victoires/meilleur->nb_simus;
	
	for(int i = 1; i<racine->nb_enfants;i++){
		if(racine->enfants[i]->nb_simus == 0)	currentVal = 0;
		else		currentVal = (double) racine->enfants[i]->nb_victoires/racine->enfants[i]->nb_simus;
		
		if(maxVal < currentVal){
			meilleur = racine->enfants[i];
			maxVal = currentVal;
		}
	}
	return meilleur;
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	Coup * meilleur_coup ;
	
	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);	
	racine->etat = copieEtat(etat); 
	
	// créer les premiers noeuds:
	coups = coups_possibles(racine->etat); 
	int k = 0;
	Noeud * enfant;
	while ( coups[k] != NULL) {
		enfant = ajouterEnfant(racine, coups[k]);
		k++;
	}
	
	
	//MCTS-UCT pour déterminer le meilleur coup

	int iter = 0;
	Noeud * n = racine;
	Noeud * current;
	Etat * new_etat;
	do {
	
	
		current = UTC(racine);
		enfant = expansion(current);
		
		new_etat = copieEtat(enfant->etat);
		propa_result(enfant,simulation(new_etat));
	
		toc = clock(); 
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;
	} while ( temps < tempsmax );
	
	meilleur_coup = meilleurFils(racine)->coup;
	
	/* fin de l'algorithme  */ 
	
	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup );
	
	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}

int main(int argc, char* argv[]) {

	Coup * coup;
	FinDePartie fin;
	
	SDL_Window* fenetre = NULL;
	SDL_Renderer* pRenderer = NULL;
	
	// initialisation
	Etat * etat = etat_initial(); 
	
	// Choisir qui commence : 
	/*printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );*/
	etat->joueur = 0;
	
	// boucle de jeu
	do {
		afficheJeu_sdl(fenetre, pRenderer, etat);
		
		if ( etat->joueur == 0 ) {
			// tour de l'humain
			
			do {
				coup = demanderCoup(etat);
			} while ( !jouerCoup(etat, coup) );
									
		}
		else {
			// tour de l'Ordinateur
			
			ordijoue_mcts( etat, TEMPS );
			//todo : effacer
			etat->joueur = 0;
		}
		
		fin = testFin( etat );
	}	while ( fin == NON );

	printf("\n");
	//afficheJeu_sdl(fenetre, pRenderer,etat);
	
	//Fermeture des éléments de la fenêtre graphique
	
	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(fenetre);
	SDL_Quit();
		
	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
