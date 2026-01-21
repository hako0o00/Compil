// Fichier regex_to_dfa.c
// Implementation complete des algorithmes du Chapitre II
// Conforme au cours de Techniques de Compilation

#include "regex_to_dfa.h"

// ========== GESTION DES ENSEMBLES D'ETATS ==========

void initStateSet(StateSet *set) {
    set->count = 0;
    memset(set->states, 0, sizeof(set->states));
}

void addToStateSet(StateSet *set, int state) {
    if (!containsState(set, state)) {
        set->states[set->count++] = state;
    }
}

bool containsState(StateSet *set, int state) {
    for (int i = 0; i < set->count; i++) {
        if (set->states[i] == state) {
            return true;
        }
    }
    return false;
}

bool stateSetsEqual(StateSet *set1, StateSet *set2) {
    if (set1->count != set2->count) return false;
    
    for (int i = 0; i < set1->count; i++) {
        if (!containsState(set2, set1->states[i])) {
            return false;
        }
    }
    return true;
}

void copyStateSet(StateSet *dest, StateSet *src) {
    dest->count = src->count;
    memcpy(dest->states, src->states, sizeof(int) * src->count);
}

void printStateSet(StateSet *set) {
    printf("{");
    for (int i = 0; i < set->count; i++) {
        printf("%d", set->states[i]);
        if (i < set->count - 1) printf(", ");
    }
    printf("}");
}

int findStateSetIndex(DFA *dfa, StateSet *set) {
    for (int i = 0; i < dfa->num_states; i++) {
        if (stateSetsEqual(&dfa->state_sets[i], set)) {
            return i;
        }
    }
    return -1;
}

// ========== GESTION DU NFA ==========

void initNFA(NFA *nfa) {
    nfa->num_states = 0;
    nfa->initial_state = 0;
    nfa->final_state = 0;
    nfa->num_transitions = 0;
    nfa->alphabet_size = 0;
    memset(nfa->alphabet, 0, sizeof(nfa->alphabet));
}

void addNFATransition(NFA *nfa, int from, int to, char symbol) {
    if (nfa->num_transitions >= MAX_TRANSITIONS) {
        fprintf(stderr, "Erreur: trop de transitions NFA\n");
        return;
    }
    
    nfa->transitions[nfa->num_transitions].from_state = from;
    nfa->transitions[nfa->num_transitions].to_state = to;
    nfa->transitions[nfa->num_transitions].symbol = symbol;
    nfa->num_transitions++;
    
    if (symbol != EPSILON) {
        addToAlphabet(nfa, symbol);
    }
}

void addToAlphabet(NFA *nfa, char symbol) {
    for (int i = 0; i < nfa->alphabet_size; i++) {
        if (nfa->alphabet[i] == symbol) return;
    }
    if (nfa->alphabet_size < MAX_SYMBOLS) {
        nfa->alphabet[nfa->alphabet_size++] = symbol;
    }
}

void printNFA(NFA *nfa) {
    printf("\n=== NFA ===\n");
    printf("Nombre d'etats: %d\n", nfa->num_states);
    printf("Etat initial: %d\n", nfa->initial_state);
    printf("Etat final: %d\n", nfa->final_state);
    printf("Alphabet: {");
    for (int i = 0; i < nfa->alphabet_size; i++) {
        printf("%c", nfa->alphabet[i]);
        if (i < nfa->alphabet_size - 1) printf(", ");
    }
    printf("}\n");
    
    printf("\nTransitions:\n");
    for (int i = 0; i < nfa->num_transitions; i++) {
        printf("  δ(%d, %c) -> %d\n",
               nfa->transitions[i].from_state,
               nfa->transitions[i].symbol == EPSILON ? 'ε' : nfa->transitions[i].symbol,
               nfa->transitions[i].to_state);
    }
}

// ========== CONSTRUCTION DE THOMPSON ==========

NFA *createBasicNFA(char c) {
    NFA *nfa = (NFA *)malloc(sizeof(NFA));
    initNFA(nfa);
    
    nfa->num_states = 2;
    nfa->initial_state = 0;
    nfa->final_state = 1;
    
    addNFATransition(nfa, 0, 1, c);
    
    return nfa;
}

NFA *createEpsilonNFA() {
    NFA *nfa = (NFA *)malloc(sizeof(NFA));
    initNFA(nfa);
    
    nfa->num_states = 2;
    nfa->initial_state = 0;
    nfa->final_state = 1;
    
    addNFATransition(nfa, 0, 1, EPSILON);
    
    return nfa;
}

NFA *concatenateNFA(NFA *nfa1, NFA *nfa2) {
    NFA *result = (NFA *)malloc(sizeof(NFA));
    initNFA(result);
    
    int offset = nfa1->num_states;
    result->num_states = nfa1->num_states + nfa2->num_states;
    result->initial_state = nfa1->initial_state;
    result->final_state = offset + nfa2->final_state;
    
    // Copier transitions de nfa1
    for (int i = 0; i < nfa1->num_transitions; i++) {
        addNFATransition(result,
                        nfa1->transitions[i].from_state,
                        nfa1->transitions[i].to_state,
                        nfa1->transitions[i].symbol);
    }
    
    // Ajouter epsilon-transition entre nfa1.final et nfa2.initial
    addNFATransition(result, nfa1->final_state, offset + nfa2->initial_state, EPSILON);
    
    // Copier transitions de nfa2 (avec offset)
    for (int i = 0; i < nfa2->num_transitions; i++) {
        addNFATransition(result,
                        offset + nfa2->transitions[i].from_state,
                        offset + nfa2->transitions[i].to_state,
                        nfa2->transitions[i].symbol);
    }
    
    free(nfa1);
    free(nfa2);
    
    return result;
}

NFA *unionNFA(NFA *nfa1, NFA *nfa2) {
    NFA *result = (NFA *)malloc(sizeof(NFA));
    initNFA(result);
    
    int offset1 = 1;
    int offset2 = offset1 + nfa1->num_states;
    result->num_states = nfa1->num_states + nfa2->num_states + 2;
    result->initial_state = 0;
    result->final_state = result->num_states - 1;
    
    // Transitions depuis nouvel etat initial
    addNFATransition(result, 0, offset1 + nfa1->initial_state, EPSILON);
    addNFATransition(result, 0, offset2 + nfa2->initial_state, EPSILON);
    
    // Copier transitions de nfa1 (avec offset1)
    for (int i = 0; i < nfa1->num_transitions; i++) {
        addNFATransition(result,
                        offset1 + nfa1->transitions[i].from_state,
                        offset1 + nfa1->transitions[i].to_state,
                        nfa1->transitions[i].symbol);
    }
    
    // Copier transitions de nfa2 (avec offset2)
    for (int i = 0; i < nfa2->num_transitions; i++) {
        addNFATransition(result,
                        offset2 + nfa2->transitions[i].from_state,
                        offset2 + nfa2->transitions[i].to_state,
                        nfa2->transitions[i].symbol);
    }
    
    // Transitions vers nouvel etat final
    addNFATransition(result, offset1 + nfa1->final_state, result->final_state, EPSILON);
    addNFATransition(result, offset2 + nfa2->final_state, result->final_state, EPSILON);
    
    free(nfa1);
    free(nfa2);
    
    return result;
}

NFA *starNFA(NFA *nfa) {
    NFA *result = (NFA *)malloc(sizeof(NFA));
    initNFA(result);
    
    int offset = 1;
    result->num_states = nfa->num_states + 2;
    result->initial_state = 0;
    result->final_state = result->num_states - 1;
    
    // Epsilon-transition: nouvel initial -> ancien initial
    addNFATransition(result, 0, offset + nfa->initial_state, EPSILON);
    
    // Epsilon-transition: nouvel initial -> nouvel final (pour accepter epsilon)
    addNFATransition(result, 0, result->final_state, EPSILON);
    
    // Copier transitions de nfa (avec offset)
    for (int i = 0; i < nfa->num_transitions; i++) {
        addNFATransition(result,
                        offset + nfa->transitions[i].from_state,
                        offset + nfa->transitions[i].to_state,
                        nfa->transitions[i].symbol);
    }
    
    // Epsilon-transition: ancien final -> ancien initial (boucle)
    addNFATransition(result, offset + nfa->final_state, offset + nfa->initial_state, EPSILON);
    
    // Epsilon-transition: ancien final -> nouvel final
    addNFATransition(result, offset + nfa->final_state, result->final_state, EPSILON);
    
    free(nfa);
    
    return result;
}

// ========== PARSING D'EXPRESSION REGULIERE ==========

RegexNode *createNode(NodeType type, char value, RegexNode *left, RegexNode *right) {
    RegexNode *node = (RegexNode *)malloc(sizeof(RegexNode));
    node->type = type;
    node->value = value;
    node->left = left;
    node->right = right;
    return node;
}

RegexNode *parsePrimary(const char **regex) {
    if (**regex == '(') {
        (*regex)++;  // Skip '('
        RegexNode *node = parseUnion(regex);
        if (**regex == ')') {
            (*regex)++;  // Skip ')'
        }
        return node;
    } else if (**regex == '!') {
        (*regex)++;
        return createNode(EPSILON_NODE, EPSILON, NULL, NULL);
    } else if (isalnum(**regex) || **regex == '_') {
        char c = **regex;
        (*regex)++;
        return createNode(CHAR_NODE, c, NULL, NULL);
    }
    return NULL;
}

RegexNode *parseStar(const char **regex) {
    RegexNode *node = parsePrimary(regex);
    
    while (**regex == '*' || **regex == '+') {
        char op = **regex;
        (*regex)++;
        
        if (op == '*') {
            node = createNode(STAR_NODE, 0, node, NULL);
        } else if (op == '+') {
            // a+ = a.a*
            RegexNode *starNode = createNode(STAR_NODE, 0, 
                                             createNode(node->type, node->value, 
                                                       node->left, node->right), 
                                             NULL);
            node = createNode(CONCAT_NODE, 0, node, starNode);
        }
    }
    
    return node;
}

RegexNode *parseConcat(const char **regex) {
    RegexNode *left = parseStar(regex);
    
    while (**regex && **regex != ')' && **regex != '|') {
        RegexNode *right = parseStar(regex);
        if (right) {
            left = createNode(CONCAT_NODE, 0, left, right);
        } else {
            break;
        }
    }
    
    return left;
}

RegexNode *parseUnion(const char **regex) {
    RegexNode *left = parseConcat(regex);
    
    while (**regex == '|') {
        (*regex)++;  // Skip '|'
        RegexNode *right = parseConcat(regex);
        left = createNode(UNION_NODE, 0, left, right);
    }
    
    return left;
}

RegexNode *parseRegex(const char *regex) {
    return parseUnion(&regex);
}

void freeRegexTree(RegexNode *node) {
    if (node == NULL) return;
    freeRegexTree(node->left);
    freeRegexTree(node->right);
    free(node);
}

NFA *regexTreeToNFA(RegexNode *node) {
    if (node == NULL) return NULL;
    
    switch (node->type) {
        case CHAR_NODE:
            return createBasicNFA(node->value);
            
        case EPSILON_NODE:
            return createEpsilonNFA();
            
        case CONCAT_NODE: {
            NFA *left = regexTreeToNFA(node->left);
            NFA *right = regexTreeToNFA(node->right);
            return concatenateNFA(left, right);
        }
        
        case UNION_NODE: {
            NFA *left = regexTreeToNFA(node->left);
            NFA *right = regexTreeToNFA(node->right);
            return unionNFA(left, right);
        }
        
        case STAR_NODE: {
            NFA *nfa = regexTreeToNFA(node->left);
            return starNFA(nfa);
        }
    }
    
    return NULL;
}

// ========== ε-FERMETURE ==========

void epsilonClosureSingle(NFA *nfa, int state, StateSet *result) {
    addToStateSet(result, state);
    
    for (int i = 0; i < nfa->num_transitions; i++) {
        if (nfa->transitions[i].from_state == state &&
            nfa->transitions[i].symbol == EPSILON &&
            !containsState(result, nfa->transitions[i].to_state)) {
            epsilonClosureSingle(nfa, nfa->transitions[i].to_state, result);
        }
    }
}

void epsilonClosure(NFA *nfa, StateSet *states, StateSet *result) {
    initStateSet(result);
    
    for (int i = 0; i < states->count; i++) {
        epsilonClosureSingle(nfa, states->states[i], result);
    }
}

// ========== TRANSITION NFA ==========

void nfaTransition(NFA *nfa, StateSet *states, char symbol, StateSet *result) {
    initStateSet(result);
    
    for (int i = 0; i < states->count; i++) {
        int current_state = states->states[i];
        
        for (int j = 0; j < nfa->num_transitions; j++) {
            if (nfa->transitions[j].from_state == current_state &&
                nfa->transitions[j].symbol == symbol) {
                addToStateSet(result, nfa->transitions[j].to_state);
            }
        }
    }
}

// ========== CONVERSION NFA -> DFA ==========

DFA *nfaToDFA(NFA *nfa) {
    DFA *dfa = (DFA *)malloc(sizeof(DFA));
    dfa->num_states = 0;
    dfa->num_transitions = 0;
    dfa->alphabet_size = nfa->alphabet_size;
    memcpy(dfa->alphabet, nfa->alphabet, sizeof(char) * nfa->alphabet_size);
    
    // File pour les etats non marques
    StateSet queue[MAX_STATES];
    bool marked[MAX_STATES] = {false};
    int queue_front = 0, queue_back = 0;
    
    // Etat initial du DFA = ε-fermeture(etat initial NFA)
    StateSet initial_set;
    initStateSet(&initial_set);
    addToStateSet(&initial_set, nfa->initial_state);
    
    StateSet initial_closure;
    epsilonClosure(nfa, &initial_set, &initial_closure);
    
    copyStateSet(&dfa->state_sets[0], &initial_closure);
    dfa->num_states = 1;
    dfa->initial_state = 0;
    
    // Verifier si l'etat initial contient l'etat final du NFA
    dfa->final_states[0] = containsState(&initial_closure, nfa->final_state);
    
    queue[queue_back++] = initial_closure;
    
    // Construction des etats DFA
    while (queue_front < queue_back) {
        StateSet current = queue[queue_front];
        int current_index = findStateSetIndex(dfa, &current);
        marked[current_index] = true;
        queue_front++;
        
        // Pour chaque symbole de l'alphabet
        for (int i = 0; i < dfa->alphabet_size; i++) {
            char symbol = dfa->alphabet[i];
            
            StateSet transition_result;
            nfaTransition(nfa, &current, symbol, &transition_result);
            
            StateSet closure_result;
            epsilonClosure(nfa, &transition_result, &closure_result);
            
            if (closure_result.count > 0) {
                int target_index = findStateSetIndex(dfa, &closure_result);
                
                if (target_index == -1) {
                    // Nouvel etat
                    target_index = dfa->num_states;
                    copyStateSet(&dfa->state_sets[target_index], &closure_result);
                    dfa->final_states[target_index] = containsState(&closure_result, nfa->final_state);
                    dfa->num_states++;
                    
                    queue[queue_back++] = closure_result;
                }
                
                // Ajouter transition
                dfa->transitions[dfa->num_transitions].from_state = current_index;
                dfa->transitions[dfa->num_transitions].to_state = target_index;
                dfa->transitions[dfa->num_transitions].symbol = symbol;
                dfa->num_transitions++;
            }
        }
    }
    
    return dfa;
}

void printDFA(DFA *dfa) {
    printf("\n=== DFA ===\n");
    printf("Nombre d'etats: %d\n", dfa->num_states);
    printf("Etat initial: %d ", dfa->initial_state);
    printStateSet(&dfa->state_sets[dfa->initial_state]);
    printf("\n");
    
    printf("Etats finaux: ");
    for (int i = 0; i < dfa->num_states; i++) {
        if (dfa->final_states[i]) {
            printf("%d ", i);
            printStateSet(&dfa->state_sets[i]);
            printf(" ");
        }
    }
    printf("\n");
    
    printf("Alphabet: {");
    for (int i = 0; i < dfa->alphabet_size; i++) {
        printf("%c", dfa->alphabet[i]);
        if (i < dfa->alphabet_size - 1) printf(", ");
    }
    printf("}\n");
    
    printf("\nEtats DFA (ensembles d'etats NFA):\n");
    for (int i = 0; i < dfa->num_states; i++) {
        printf("  S%d = ", i);
        printStateSet(&dfa->state_sets[i]);
        if (dfa->final_states[i]) printf(" (final)");
        printf("\n");
    }
    
    printf("\nTransitions:\n");
    for (int i = 0; i < dfa->num_transitions; i++) {
        printf("  δ(S%d, %c) -> S%d\n",
               dfa->transitions[i].from_state,
               dfa->transitions[i].symbol,
               dfa->transitions[i].to_state);
    }
}

// ========== MINIMISATION DE DFA ==========

DFA *minimizeDFA(DFA *dfa) {
    printf("\n=== MINIMISATION DU DFA ===\n");
    
    // Partition initiale: etats finaux vs non-finaux
    int partition[MAX_STATES];
    int num_groups = 2;
    
    for (int i = 0; i < dfa->num_states; i++) {
        partition[i] = dfa->final_states[i] ? 1 : 0;
    }
    
    printf("Partition initiale:\n");
    printf("  Groupe 0 (non-finaux): ");
    for (int i = 0; i < dfa->num_states; i++) {
        if (partition[i] == 0) printf("S%d ", i);
    }
    printf("\n  Groupe 1 (finaux): ");
    for (int i = 0; i < dfa->num_states; i++) {
        if (partition[i] == 1) printf("S%d ", i);
    }
    printf("\n");
    
    bool changed = true;
    int iteration = 0;
    
    while (changed) {
        changed = false;
        iteration++;
        printf("\nIteration %d:\n", iteration);
        
        int new_partition[MAX_STATES];
        memcpy(new_partition, partition, sizeof(partition));
        int next_group = num_groups;
        
        for (int g = 0; g < num_groups; g++) {
            // Trouver tous les etats du groupe g
            int group_states[MAX_STATES];
            int group_size = 0;
            
            for (int i = 0; i < dfa->num_states; i++) {
                if (partition[i] == g) {
                    group_states[group_size++] = i;
                }
            }
            
            if (group_size <= 1) continue;
            
            // Verifier si les etats du groupe doivent etre divises
            for (int i = 1; i < group_size; i++) {
                int s1 = group_states[0];
                int s2 = group_states[i];
                bool should_split = false;
                
                // Comparer les transitions pour chaque symbole
                for (int a = 0; a < dfa->alphabet_size; a++) {
                    char symbol = dfa->alphabet[a];
                    
                    int target1 = -1, target2 = -1;
                    
                    // Trouver transition pour s1
                    for (int t = 0; t < dfa->num_transitions; t++) {
                        if (dfa->transitions[t].from_state == s1 &&
                            dfa->transitions[t].symbol == symbol) {
                            target1 = dfa->transitions[t].to_state;
                            break;
                        }
                    }
                    
                    // Trouver transition pour s2
                    for (int t = 0; t < dfa->num_transitions; t++) {
                        if (dfa->transitions[t].from_state == s2 &&
                            dfa->transitions[t].symbol == symbol) {
                            target2 = dfa->transitions[t].to_state;
                            break;
                        }
                    }
                    
                    // Comparer les groupes des cibles
                    if ((target1 == -1) != (target2 == -1) ||
                        (target1 != -1 && partition[target1] != partition[target2])) {
                        should_split = true;
                        break;
                    }
                }
                
                if (should_split) {
                    new_partition[s2] = next_group;
                    changed = true;
                }
            }
            
            if (changed) {
                next_group++;
            }
        }
        
        if (changed) {
            memcpy(partition, new_partition, sizeof(partition));
            num_groups = next_group;
            
            printf("Nouvelle partition:\n");
            for (int g = 0; g < num_groups; g++) {
                printf("  Groupe %d: ", g);
                for (int i = 0; i < dfa->num_states; i++) {
                    if (partition[i] == g) printf("S%d ", i);
                }
                printf("\n");
            }
        }
    }
    
    printf("\nDFA minimal obtenu avec %d etats.\n", num_groups);
    
    // Construire le DFA minimal
    DFA *min_dfa = (DFA *)malloc(sizeof(DFA));
    min_dfa->num_states = num_groups;
    min_dfa->alphabet_size = dfa->alphabet_size;
    memcpy(min_dfa->alphabet, dfa->alphabet, sizeof(char) * dfa->alphabet_size);
    min_dfa->num_transitions = 0;
    
    // Determiner l'etat initial du DFA minimal
    min_dfa->initial_state = partition[dfa->initial_state];
    
    // Determiner les etats finaux
    for (int i = 0; i < num_groups; i++) {
        min_dfa->final_states[i] = false;
    }
    for (int i = 0; i < dfa->num_states; i++) {
        if (dfa->final_states[i]) {
            min_dfa->final_states[partition[i]] = true;
        }
    }
    
    // Construire les transitions (sans doublons)
    bool added[MAX_STATES][MAX_SYMBOLS] = {false};
    
    for (int i = 0; i < dfa->num_transitions; i++) {
        int from_group = partition[dfa->transitions[i].from_state];
        int to_group = partition[dfa->transitions[i].to_state];
        char symbol = dfa->transitions[i].symbol;
        
        // Trouver l'index du symbole
        int symbol_idx = 0;
        for (int j = 0; j < dfa->alphabet_size; j++) {
            if (dfa->alphabet[j] == symbol) {
                symbol_idx = j;
                break;
            }
        }
        
        if (!added[from_group][symbol_idx]) {
            min_dfa->transitions[min_dfa->num_transitions].from_state = from_group;
            min_dfa->transitions[min_dfa->num_transitions].to_state = to_group;
            min_dfa->transitions[min_dfa->num_transitions].symbol = symbol;
            min_dfa->num_transitions++;
            added[from_group][symbol_idx] = true;
        }
    }
    
    return min_dfa;
}

// ========== TABLE DE TRANSITION ==========

void printTransitionTable(DFA *dfa) {
    printf("\n=== TABLE DE TRANSITION ===\n");
    printf("%-10s", "Etat");
    for (int i = 0; i < dfa->alphabet_size; i++) {
        printf("%-10c", dfa->alphabet[i]);
    }
    printf("\n");
    
    for (int i = 0; i < 50; i++) printf("-");
    printf("\n");
    
    for (int i = 0; i < dfa->num_states; i++) {
        if (i == dfa->initial_state && dfa->final_states[i]) {
            printf("->*S%-6d", i);
        } else if (i == dfa->initial_state) {
            printf("->S%-7d", i);
        } else if (dfa->final_states[i]) {
            printf("*S%-8d", i);
        } else {
            printf("S%-9d", i);
        }
        
        for (int j = 0; j < dfa->alphabet_size; j++) {
            char symbol = dfa->alphabet[j];
            bool found = false;
            
            for (int t = 0; t < dfa->num_transitions; t++) {
                if (dfa->transitions[t].from_state == i &&
                    dfa->transitions[t].symbol == symbol) {
                    printf("S%-9d", dfa->transitions[t].to_state);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                printf("%-10s", "-");
            }
        }
        printf("\n");
    }
}

// ========== SIMULATION DE DFA ==========

bool simulateDFA(DFA *dfa, const char *input) {
    int current_state = dfa->initial_state;
    
    printf("\n=== SIMULATION DU DFA ===\n");
    printf("Chaine d'entree: \"%s\"\n\n", input);
    printf("%-15s%-15s%-15s\n", "Etat actuel", "Symbole lu", "Nouvel etat");
    printf("-----------------------------------------------\n");
    
    for (int i = 0; input[i] != '\0'; i++) {
        char symbol = input[i];
        int next_state = -1;
        
        // Trouver la transition
        for (int t = 0; t < dfa->num_transitions; t++) {
            if (dfa->transitions[t].from_state == current_state &&
                dfa->transitions[t].symbol == symbol) {
                next_state = dfa->transitions[t].to_state;
                break;
            }
        }
        
        printf("S%-14d%-15c", current_state, symbol);
        
        if (next_state == -1) {
            printf("ERREUR\n");
            printf("\nChaine REJETEE: pas de transition pour '%c' depuis S%d\n", symbol, current_state);
            return false;
        }
        
        printf("S%-14d\n", next_state);
        current_state = next_state;
    }
    
    printf("\nEtat final: S%d ", current_state);
    if (dfa->final_states[current_state]) {
        printf("(etat d'acceptation)\n");
        printf("Chaine ACCEPTEE\n");
        return true;
    } else {
        printf("(etat non-acceptant)\n");
        printf("Chaine REJETEE\n");
        return false;
    }
}

// ========== FONCTION DE DEMONSTRATION ==========

void demonstrateRegexToDFA(const char *regex) {
    printf("\n");
    printf("================================================================================\n");
    printf("     DEMONSTRATION: TRANSFORMATION EXPRESSION REGULIERE -> NFA -> DFA\n");
    printf("     Conforme au Chapitre II du cours de Techniques de Compilation\n");
    printf("================================================================================\n");
    printf("\nExpression reguliere: %s\n", regex);
    
    // Etape 1: Parser l'expression reguliere
    printf("\n--- ETAPE 1: ANALYSE DE L'EXPRESSION REGULIERE ---\n");
    RegexNode *tree = parseRegex(regex);
    if (tree == NULL) {
        printf("Erreur: impossible de parser l'expression reguliere\n");
        return;
    }
    printf("Arbre d'analyse construit avec succes.\n");
    
    // Etape 2: Construction de Thompson (Regex -> NFA)
    printf("\n--- ETAPE 2: CONSTRUCTION DE THOMPSON (REGEX -> NFA) ---\n");
    NFA *nfa = regexTreeToNFA(tree);
    if (nfa == NULL) {
        printf("Erreur: impossible de construire le NFA\n");
        freeRegexTree(tree);
        return;
    }
    printNFA(nfa);
    
    // Etape 3: Construction de sous-ensembles (NFA -> DFA)
    printf("\n--- ETAPE 3: CONSTRUCTION DE SOUS-ENSEMBLES (NFA -> DFA) ---\n");
    DFA *dfa = nfaToDFA(nfa);
    printDFA(dfa);
    printTransitionTable(dfa);
    
    // Etape 4: Minimisation du DFA
    printf("\n--- ETAPE 4: MINIMISATION DU DFA ---\n");
    DFA *min_dfa = minimizeDFA(dfa);
    printDFA(min_dfa);
    printTransitionTable(min_dfa);
    
    // Nettoyer
    freeRegexTree(tree);
    free(nfa);
    free(dfa);
    free(min_dfa);
}
