typedef int IntegerOid;  // Para existir a aproximação máxima à escrita de uma MIB definimos estas variáveis. -> Pode ser substituido por um Int
typedef char *OctetStringOid;

typedef struct {
  IntegerOid keyId;
  OctetStringOid keyValue;
  OctetStringOid keyRequester;
  IntegerOid keyExpirationDate;
  IntegerOid keyExpirationTime;
  IntegerOid keyVisibility;
} DataTableGeneratedKeysEntry;

typedef union // Union é basicamente uma coleção de variáveis de tipos diferentes numa unica posiçãode memória, logo é uma boa maneira de guardar os valores que chegam à MIB
{   //Apesar da uma verdadeira MIB não ser usada para guardar diretamente os dados, nós vamos aproveitar a mesma para esse fim.
    DataTableGeneratedKeysEntry Entry;
    char *Stringvalue;
    int IntValue;
} Mib_value;

struct ObjectType{
  IntegerOid *oid;
  char *name;
  int syntax;
  int maxAccess;
  int status;
  char *description;
  Mib_value value; //Guardamos o valor em char já que este equivale a um byte em C e é a maneira mais facil de guardar os valores recebidos
};

typedef struct{
    struct hashmap *Mib;
} dados_MibAgent;

typedef struct {
    int K_T;  // Basicamente K
    int T;    // Tempo entre atualizações
    unsigned char *Z_S;
} dados_KeyAgent;

typedef struct {
    char *IP_add;
    int port;
} dados_CommAgent;

typedef struct {
  int temp;
} dados_ProtocolAgent;