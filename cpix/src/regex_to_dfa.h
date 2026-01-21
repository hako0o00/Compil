// Fichier regex_to_dfa.h
// Implementation des transformations: Expression Reguliere -> NFA -> DFA -> DFA Minimal
// Conforme au cours de Techniques de Compilation (Chapitre II)

#ifndef REGEX_TO_DFA_H
#define REGEX_TO_DFA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Constantes
#define MAX_STATES 100
#define MAX_SYMBOLS 50
#define MAX_TRANSITIONS 200
#define MAX_EXPR_LEN 500
#define EPSILON 'E'  // Symbole epsilon (E pour epsilon)

// Structure pour representer une transition NFA
typedef struct {
    int from_state;
    int to_state;
    char symbol;  // EPSILON pour ε-transition
} Transition;

// Structure pour representer un NFA (Automate Fini Non Deterministe)
typedef struct {
    int num_states;
    int initial_state;
    int final_state;
    Transition transitions[MAX_TRANSITIONS];
    int num_transitions;
    char alphabet[MAX_SYMBOLS];
    int alphabet_size;
} NFA;

// Structure pour representer un ensemble d'etats (pour la construction de DFA)
typedef struct {
    int states[MAX_STATES];
    int count;
} StateSet;

// Structure pour representer une transition DFA
typedef struct {
    int from_state;
    int to_state;
    char symbol;
} DFATransition;

// Structure pour representer un DFA (Automate Fini Deterministe)
typedef struct {
    int num_states;
    StateSet state_sets[MAX_STATES];  // Chaque etat DFA est un ensemble d'etats NFA
    int initial_state;
    bool final_states[MAX_STATES];
    DFATransition transitions[MAX_TRANSITIONS];
    int num_transitions;
    char alphabet[MAX_SYMBOLS];
    int alphabet_size;
} DFA;

// Structure pour l'arbre d'expression reguliere
typedef enum {
    CHAR_NODE,      // Caractere simple
    EPSILON_NODE,   // ε
    CONCAT_NODE,    // Concatenation
    UNION_NODE,     // Union |
    STAR_NODE       // Kleene star *
} NodeType;

typedef struct RegexNode {
    NodeType type;
    char value;  // Pour CHAR_NODE et EPSILON_NODE
    struct RegexNode *left;
    struct RegexNode *right;
} RegexNode;

// ========== Fonctions pour les ensembles d'etats ==========
void initStateSet(StateSet *set);
void addToStateSet(StateSet *set, int state);
bool containsState(StateSet *set, int state);
bool stateSetsEqual(StateSet *set1, StateSet *set2);
void copyStateSet(StateSet *dest, StateSet *src);
void printStateSet(StateSet *set);
int findStateSetIndex(DFA *dfa, StateSet *set);

// ========== Fonctions pour NFA ==========
void initNFA(NFA *nfa);
void addNFATransition(NFA *nfa, int from, int to, char symbol);
void printNFA(NFA *nfa);
void addToAlphabet(NFA *nfa, char symbol);

// ========== Construction de Thompson ==========
NFA *createBasicNFA(char c);
NFA *createEpsilonNFA();
NFA *concatenateNFA(NFA *nfa1, NFA *nfa2);
NFA *unionNFA(NFA *nfa1, NFA *nfa2);
NFA *starNFA(NFA *nfa);

// ========== Parsing d'expression reguliere ==========
RegexNode *parseRegex(const char *regex);
RegexNode *parseUnion(const char **regex);
RegexNode *parseConcat(const char **regex);
RegexNode *parseStar(const char **regex);
RegexNode *parsePrimary(const char **regex);
NFA *regexTreeToNFA(RegexNode *node);
void freeRegexTree(RegexNode *node);

// ========== ε-fermeture ==========
void epsilonClosure(NFA *nfa, StateSet *states, StateSet *result);
void epsilonClosureSingle(NFA *nfa, int state, StateSet *result);

// ========== Transition NFA ==========
void nfaTransition(NFA *nfa, StateSet *states, char symbol, StateSet *result);

// ========== Conversion NFA -> DFA (Construction de sous-ensembles) ==========
DFA *nfaToDFA(NFA *nfa);
void printDFA(DFA *dfa);

// ========== Minimisation de DFA ==========
DFA *minimizeDFA(DFA *dfa);
void printDFAMinimal(DFA *dfa);

// ========== Table de transition ==========
void printTransitionTable(DFA *dfa);

// ========== Simulation de DFA ==========
bool simulateDFA(DFA *dfa, const char *input);

// ========== Fonction principale de demonstration ==========
void demonstrateRegexToDFA(const char *regex);

#endif // REGEX_TO_DFA_H
