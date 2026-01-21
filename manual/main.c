// Fichier main.c - Analyseur LL(1) pour cpixel
#include "analyseur.h"

int main()
{
    int i, j;
    char STR[MAX];
    char line[MAX];
    FILE *file;

    // Définition de la grammaire pour cpixel (structure réelle)
    NbNonTerm = 4; // Nombre de non-terminaux
    NT[0] = 'P';   // Program
    NT[1] = 'D';   // Declaration
    NT[2] = 'B';   // Block (renamed to avoid conflict with START terminal)
    NT[3] = 'T';   // Statement
    NbTerm = 11;   // Nombre de terminaux
    T[0] = 't';    // TYPE_INT
    T[1] = 'i';    // IDENT
    T[2] = '=';    // ASSIGN
    T[3] = 'n';    // INTNUM
    T[4] = 's';    // START (changed to lowercase)
    T[5] = '{';    // LBRACE
    T[6] = 'p';    // PRINT
    T[7] = '(';    // LPAREN
    T[8] = ')';    // RPAREN
    T[9] = '}';    // RBRACE
    T[10] = '#';   // Symbole de fin

    char x_lexer_grammaire[MAX][MAX] = {
        "Program -> Declaration StartBlock",
        "Declaration -> TYPE_INT IDENT ASSIGN INTNUM",
        "StartBlock -> START LBRACE Statement RBRACE",
        "Statement -> PRINT LPAREN IDENT RPAREN"};
    strcpy(G[0], "P->DB");
    strcpy(G[1], "D->ti=n");
    strcpy(G[2], "B->s{T}");
    strcpy(G[3], "T->p(i)");
    NbRegles = 4; // Nombre de règles de grammaire
    NbRegles = 4; // Nombre de règles de grammaire

    // Affichage de la grammaire cpixel
    printf("\n*************************** GRAMMAIRE CPIXEL ***************************\n");
    printf("Regles de production :\n");
    for (int i = 0; i < NbRegles; i++)
    {
        printf("\t%d. %s\n", i + 1, x_lexer_grammaire[i]);
    }
    printf("************************ FIN GRAMMAIRE CPIXEL ************************\n\n");

    if (Verifications())
    {
        afficher_debut();   // Affichage des ensembles DEBUT
        afficher_suivant(); // Affichage des ensembles SUIVANT
        if (verifier_intersection_debuts())
        {
            printf(">> La grammaire n'est pas LL(1)\n");
            return 0;
        }
        else
        {
            creer_table_analyse();
            afficher_table_analyse();

            // Le fichier contient la chaîne à analyser
            file = fopen("cpixel_test.txt", "r");
            if (file == NULL)
            {
                printf("Erreur d'ouverture du fichier cpixel_test.txt\n");
                return 0;
            }

            // Lire le contenu du fichier dans une seule variable
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            char *file_content = malloc(file_size + 1);
            fread(file_content, 1, file_size, file);
            file_content[file_size] = '\0';
            // Convertir les tokens en symboles
            char converted_string[MAX] = "";
            convertir_tokens(file_content, converted_string);
            strcpy(STR, converted_string); // Chaîne à analyser
            printf("Chaine a analyser : %s\n", file_content);
            analyseDescLL1(STR);
            free(file_content); // Libérer la mémoire allouée
            fclose(file);
            return 0;
        }
    }
    else
    {
        printf(">> La grammaire n'est pas LL(1)\n");
        return 0;
    }
}
