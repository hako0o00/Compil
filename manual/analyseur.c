// Fichier analyseur.c - Analyseur LL(1) pour cpixel
#include "analyseur.h"

char DEBUT[MAX][MAX];
char SUIVANT[MAX][MAX];
char T[MAX], NT[MAX], G[MAX][MAX];
int NbTerm, NbNonTerm, ch, NbRegles = 4;
int cpt;
int top = 0;
char PILE[MAX];
int LL1[MAX][MAX];

// Fonction principale pour toutes les vérifications avant l'analyse
bool Verifications()
{
    printf("*************************** VERIFICATIONS  ***************************\n");
    bool rec_gauche = verifier_rec_gauche();
    bool factorisation = verifier_factorisation();
    bool proprete_grammaire = verifier_proprete_grammaire();
    printf("************************ FIN DES VERIFICATIONS ************************\n");
    return (!rec_gauche) && factorisation && proprete_grammaire;
}

bool verifier_rec_gauche()
{
    printf("Verification de la recursivite a gauche... \n");
    bool rec_detected = false;
    for (int i = 0; i < NbRegles; i++)
    {
        char non_terminal = G[i][0];
        if (non_terminal == G[i][3])
        {
            printf("Recursivite a gauche detectee dans la regle : %s\n", G[i]);
            printf("Veuillez corriger la grammaire pour eliminer la recursivité.\n\n");
            rec_detected = true;
        }
    }
    if (!rec_detected)
    {
        printf(">> Pas de recursivite a gauche detectee.\n");
    }
    return rec_detected;
}

bool verifier_factorisation()
{
    printf("\nVerification de factorisation...\n");
    bool is_factorised = true;
    for (int i = 0; i < NbRegles; i++)
    {
        char non_terminal = G[i][0];
        for (int j = i + 1; j < NbRegles; j++)
        {
            if (G[j][0] == non_terminal && G[i][3] == G[j][3])
            {
                printf("Ambiguite détectee : Les regles %s et %s partagent un symbole commun.\n", G[i], G[j]);
                printf("Veuillez corriger la grammaire pour eliminer la factorisation\n\n");
                is_factorised = false;
            }
        }
    }
    if (is_factorised)
    {
        printf(">> Pas d'ambiguite detectee.\n");
    }
    return is_factorised;
}

bool verifier_proprete_grammaire()
{
    printf("\nVerification si la grammaire est propre...\n");
    bool propre = true;
    for (int i = 0; i < NbRegles; i++)
    {
        if (strlen(G[i]) == 4 && isupper(G[i][3]))
        {
            printf("La regle %s n'est pas propre.\n", G[i]);
            printf("Veuillez corriger la grammaire pour eliminer les regles non propres.\n\n");
            propre = false;
        }
    }
    if (propre)
    {
        printf(">> La grammaire est propre.\n");
    }
    return propre;
}

// Les debuts des non-terminaux sont calculés et affichés
void afficher_debut()
{
    int i, j;
    char arr[MAX];
    for (i = 0; i < NbNonTerm; i++)
    {
        arr[0] = '\0';
        calculer_debut(arr, NT[i]);
        for (j = 0; arr[j] != '\0'; j++)
        {
            DEBUT[i][j] = arr[j]; // Sauvegarder l'ensemble DEBUT
        }
        DEBUT[i][j] = '\0';
        cpt = 0;
    }

    printf("\n********************* L'ENSEMBLE DES DEBUTS *********************\n");
    for (i = 0; i < NbNonTerm; i++)
    {
        printf("DEBUT( %c ): { ", NT[i]);
        for (j = 0; DEBUT[i][j + 1] != '\0'; j++)
            printf(" %c,", DEBUT[i][j]);
        printf(" %c }", DEBUT[i][j]);
        printf("\n");
    }
    printf("*******************  FIN ENSEMBLE DES DEBUTS *******************\n");
}

// Calcul de l'ensemble DEBUT pour un non-terminal
void calculer_debut(char *arr, char ch)
{
    int i;
    if (!isupper(ch)) // Si le caractère est un terminal
        ajouter_symbole(arr, ch);
    else
    {
        for (i = 0; i < NbRegles; i++)
        {
            if (ch == G[i][0])
            {
                if (G[i][3] == '!')
                    ajouter_symbole(arr, G[i][3]);
                else
                    calculer_debut(arr, G[i][3]);
            }
        }
    }
}

// Affichage des ensembles SUIVANT pour chaque non-terminal
void afficher_suivant()
{
    int i, j;
    char arr[MAX];
    for (i = 0; i < NbNonTerm; i++)
    {
        cpt = 0;
        arr[0] = '\0';
        calculer_suivant(arr, NT[i]); // Trouver l'ensemble SUIVANT pour chaque non-terminal
        for (j = 0; arr[j] != '\0'; j++)
        {
            SUIVANT[i][j] = arr[j]; // Sauvegarder l'ensemble SUIVANT
        }
        SUIVANT[i][j] = '\0';
    }
    printf("\n********************* L'ENSEMBLE DES SUIVANTS ********************\n");
    for (i = 0; i < NbNonTerm; i++)
    {
        printf("SUIVANT( %c ): { ", NT[i]);
        for (j = 0; SUIVANT[i][j + 1] != '\0'; j++)
            printf(" %c,", SUIVANT[i][j]);
        printf(" %c }", SUIVANT[i][j]);
        printf("\n");
    }
    printf("********************* FIN ENSEMBLE DES SUIVANTS *******************\n");
}

// Calcul de l'ensemble SUIVANT pour un non-terminal
void calculer_suivant(char arr[], char ch)
{
    int i, j, k, l, fl = 1, flag = 1;
    if (ch == G[0][0]) // Si le non-terminal est l'axiome S
        ajouter_symbole(arr, '#');
    for (i = 0; i < NbRegles; i++)
    {
        for (j = 3; G[i][j] != '\0' && flag == 1; j++)
        {
            if (ch == G[i][j])
            {
                flag = 0;
                if (G[i][j + 1] != '\0' && isupper(G[i][j + 1]))
                {
                    for (k = 0; k < NbNonTerm; k++)
                    {
                        if (NT[k] == G[i][j + 1])
                        {
                            for (l = 0; DEBUT[k][l] != '\0'; l++)
                            {
                                if (DEBUT[k][l] != '\0' && DEBUT[k][l] != '!')
                                    ajouter_symbole(arr, DEBUT[k][l]);
                                if (DEBUT[k][l] == '!')
                                    fl = 0;
                            }
                            break;
                        }
                    }
                }
                else if (G[i][j + 1] != '\0' && !isupper(G[i][j + 1]))
                {
                    ajouter_symbole(arr, G[i][j + 1]);
                }
                if ((G[i][j + 1] == '\0' || fl == 0) && G[i][0] != ch)
                {
                    fl = 1;
                    calculer_suivant(arr, G[i][0]);
                }
            }
        }
    }
}

// Fonction pour ajouter un symbole à un tableau, s'il n'est pas déjà présent
void ajouter_symbole(char *arr, char ch)
{
    int i, k = 0;
    for (i = 0; arr[i] != '\0'; i++)
    {
        if (ch == arr[i])
        {
            k = 1;
            break;
        }
    }
    if (k == 0)
    {
        arr[cpt++] = ch;
        arr[cpt] = '\0';
    }
}

bool verifier_intersection_debuts()
{
    printf("\n********************* INTERSECTION DES DEBUTS *********************\n");
    bool conflict_detected = false;

    for (int i = 0; i < NbNonTerm; i++)
    {
        printf("\nNon-terminal : %c\n", NT[i]);
        bool conflict = false;

        for (int j = 0; j < NbRegles; j++)
        {
            if (G[j][0] == NT[i])
            {
                for (int k = j + 1; k < NbRegles; k++)
                {
                    if (G[k][0] == NT[i])
                    {
                        char debut_j[MAX] = {'\0'};
                        char debut_k[MAX] = {'\0'};
                        calculer_debut(debut_j, G[j][3]);
                        calculer_debut(debut_k, G[k][3]);

                        for (int l = 0; debut_j[l] != '\0'; l++)
                        {
                            if (strchr(debut_k, debut_j[l]) != NULL)
                            {
                                printf("Conflit détecté entre les productions %s et %s\n", G[j], G[k]);
                                conflict = true;
                                conflict_detected = true;
                                break;
                            }
                        }
                        if (conflict)
                            break;
                    }
                }
                if (conflict)
                    break;
            }
        }

        if (!conflict)
        {
            printf(">> Aucun conflit trouve pour le Non terminal %c.\n", NT[i]);
        }
    }

    return conflict_detected;
}

void creer_table_analyse() // Création de la table d'analyse LL(1)
{
    int i, j, k, position;
    char arr[MAX];
    for (i = 0; i < NbRegles; i++)
    {
        arr[0] = '\0';
        cpt = 0;
        calculer_debut(arr, G[i][3]);
        for (j = 0; j < cpt; j++)
        {
            if (arr[j] == '!')
            {
                calculer_suivant(arr, G[i][0]);
                break;
            }
        }
        for (k = 0; k < NbNonTerm; k++)
        {
            if (NT[k] == G[i][0])
            {
                position = k;
                break;
            }
        }
        for (j = 0; j < cpt; j++)
        {
            if (arr[j] != '!')
            {
                for (k = 0; k < NbTerm; k++)
                {
                    if (arr[j] == T[k])
                    {
                        LL1[position][k] = i + 1;
                        break;
                    }
                }
            }
        }
    }
}

// Affichage de la table d'analyse LL(1)
void afficher_table_analyse()
{
    int i, j;
    printf("\n\n******************************** Table d'analyse LL(1) **********************************\n\n\t");
    for (j = 0; j < NbTerm; j++)
    {
        printf("%-8c", T[j]);
    }
    printf("\n--------------------------------------------------------------------------------------------------");
    printf("\n\n");
    for (i = 0; i < NbNonTerm; i++)
    {
        printf("%-4c|  ", NT[i]);
        for (j = 0; j < NbTerm; j++)
        {
            if (LL1[i][j] != 0)
                printf("%-8s", G[LL1[i][j] - 1]);
            else
                printf("%-8c", '-');
        }
        printf("\n\n");
    }
}

void convertir_tokens(const char *input, char *output)
{
    char temp[1000];
    char *token;

    strcpy(temp, input);
    output[0] = '\0';

    token = strtok(temp, " \n");
    while (token != NULL)
    {
        if (strcmp(token, "TYPE_INT") == 0)
            strcat(output, "t");
        else if (strcmp(token, "IDENT") == 0)
            strcat(output, "i");
        else if (strcmp(token, "ASSIGN") == 0)
            strcat(output, "=");
        else if (strcmp(token, "INTNUM") == 0)
            strcat(output, "n");
        else if (strcmp(token, "START") == 0)
            strcat(output, "s");
        else if (strcmp(token, "LBRACE") == 0)
            strcat(output, "{");
        else if (strcmp(token, "PRINT") == 0)
            strcat(output, "p");
        else if (strcmp(token, "LPAREN") == 0)
            strcat(output, "(");
        else if (strcmp(token, "RPAREN") == 0)
            strcat(output, ")");
        else if (strcmp(token, "RBRACE") == 0)
            strcat(output, "}");

        token = strtok(NULL, " \n");
    }
    strcat(output, "#");
}

// Fonction utilitaire pour créer une chaîne de caractères répétés
void repeatChar(char c, int n, char *result)
{
    int i;
    for (i = 0; i < n; i++)
    {
        result[i] = c;
    }
    result[i] = '\0';
}

// Fonction pour afficher une chaîne avec padding
void printPadded(const char *str, int width)
{
    int len = strlen(str);
    printf("%s", str);

    char spaces[MAX_WIDTH];
    repeatChar(' ', width - len, spaces);
    printf("%s", spaces);
}

void analyseDescLL1(char *input)
{
    int inputIndex = 0, stackIndex, nonTermIndex, termIndex, ruleIndex, ruleLength;
    int actionFound = 0;
    char buffer[100]; // Buffer pour formater les chaînes

    PILE[top++] = '#';
    PILE[top] = G[0][0];

    // En-tête avec séparateur
    printf("PILE%*sRESTE DE LA CHAINE%*sACTION\n", MAX_WIDTH - 4, "", MAX_WIDTH - 17, "");
    char separator[120];
    repeatChar('-', MAX_WIDTH * 3, separator);
    printf("%s\n", separator);

    while (PILE[top] != '#')
    {
        char pileContent[MAX_WIDTH] = "";
        for (stackIndex = 0; PILE[stackIndex] != '\0'; stackIndex++)
        {
            char temp[2] = {PILE[stackIndex], '\0'};
            strcat(pileContent, temp);
        }
        printPadded(pileContent, MAX_WIDTH);

        char inputContent[MAX_WIDTH] = "";
        for (stackIndex = inputIndex; input[stackIndex] != '\0'; stackIndex++)
        {
            char temp[2] = {input[stackIndex], '\0'};
            strcat(inputContent, temp);
        }
        printPadded(inputContent, MAX_WIDTH);

        actionFound = 0;

        if (input[inputIndex] == PILE[top])
        {
            sprintf(buffer, "Depiler: %c", PILE[top]);
            printPadded(buffer, MAX_WIDTH);
            PILE[top] = '\0';
            top--;
            inputIndex++;
            actionFound = 1;
        }
        else
        {
            for (nonTermIndex = 0; nonTermIndex < NbNonTerm; nonTermIndex++)
            {
                if (PILE[top] == NT[nonTermIndex])
                    break;
            }

            for (termIndex = 0; termIndex < NbTerm; termIndex++)
            {
                if (input[inputIndex] == T[termIndex])
                    break;
            }

            ruleIndex = LL1[nonTermIndex][termIndex];

            if (ruleIndex > 0)
            {
                if (G[ruleIndex - 1][3] == '!')
                {
                    PILE[top] = '\0';
                    top--;
                }
                else
                {
                    for (ruleLength = 3; G[ruleIndex - 1][ruleLength] != '\0'; ruleLength++)
                        ;
                    PILE[top] = '\0';
                    for (ruleLength--; ruleLength > 2; ruleLength--)
                        PILE[top++] = G[ruleIndex - 1][ruleLength];
                    top--;
                }

                sprintf(buffer, "Regle: %s", G[ruleIndex - 1]);
                printPadded(buffer, MAX_WIDTH);
                actionFound = 1;
            }
        }

        if (!actionFound)
        {
            printPadded("ERREUR SYNTAXIQUE", MAX_WIDTH);
            break;
        }

        printf("\n");
    }

    // Affichage final
    char finalPile[MAX_WIDTH] = "";
    for (stackIndex = 0; PILE[stackIndex] != '\0'; stackIndex++)
    {
        char temp[2] = {PILE[stackIndex], '\0'};
        strcat(finalPile, temp);
    }
    printPadded(finalPile, MAX_WIDTH);

    char finalInput[MAX_WIDTH] = "";
    for (stackIndex = inputIndex; input[stackIndex] != '\0'; stackIndex++)
    {
        char temp[2] = {input[stackIndex], '\0'};
        strcat(finalInput, temp);
    }
    printPadded(finalInput, MAX_WIDTH);
    printf("\n\n");

    if (PILE[top] == '#' && input[inputIndex] == '#')
        printf("Analyse terminee avec succes\n");
    else
        printf("Erreur syntaxique\n");
}