// Fichier analyseur_cpixel.h
#ifndef ANALYSEUR_CPIXEL_H
#define ANALYSEUR_CPIXEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Déclarations des constantes
#define MAX 80
#define MAX_WIDTH 30

// Déclarations des variables globales
extern char DEBUT[MAX][MAX];
extern char SUIVANT[MAX][MAX];
extern char T[MAX], NT[MAX], G[MAX][MAX];
extern int NbTerm, NbNonTerm, NbRegles;
extern int cpt;
extern int top;
extern char PILE[MAX];
extern int LL1[MAX][MAX];

// Déclarations des fonctions
bool Verifications();
bool verifier_factorisation();
bool verifier_rec_gauche();
bool verifier_proprete_grammaire();
void afficher_debut();
void calculer_debut(char *, char);
void afficher_suivant();
void calculer_suivant(char *, char);
void ajouter_symbole(char *, char);
bool verifier_intersection_debuts();

// Déclarations des fonctions pour l'analyse LL(1)
void creer_table_analyse();
void afficher_table_analyse();
void convertir_tokens_cpixel(const char *, char *);
void repeatChar(char, int, char *);
void printPadded(const char *, int);
void analyseDescLL1(char *);

#endif