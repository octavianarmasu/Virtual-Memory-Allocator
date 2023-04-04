#include "vma.h"

list_t *dll_create(unsigned int data_size) {
  // functie ce creeaza o lista
  list_t *list = malloc(sizeof(*list));
  if (!list) {
    printf("malloc() failed\n");
    return NULL;
  }
  list->data_size = data_size;
  list->head = NULL;
  list->size = 0;
  return list;
}

void dll_add_nth_node(list_t *list, unsigned int n, const void *new_data) {
  // Functia adauga un nod in lista. Daca n > marimea listea, atunci se va
  // adauga pe ultima pozitie a listei.

  dll_node_t *curr = list->head;
  dll_node_t *node = NULL;
  if (n > list->size) n = list->size;
  node = malloc(sizeof(dll_node_t));
  if (!node) {
    printf("malloc() failed\n");
    return;
  }
  node->prev = NULL;
  node->next = NULL;
  node->data = malloc(list->data_size);
  if (!node->data) {
    printf("malloc() failed\n");
    return;
  }
  memcpy(node->data, new_data, list->data_size);
  if (n == 0) {
    if (list->size != 0) {
      node->next = list->head;
      list->head->prev = node;
      list->head = node;
    } else {
      list->head = node;
    }
  } else {
    for (unsigned int i = 0; i < n - 1; i++) curr = curr->next;
    dll_node_t *aux = curr->next;
    node->next = curr->next;
    curr->next = node;
    node->prev = curr;
    if (aux != NULL) {
      aux->prev = node;
    }
  }
  list->size++;
}

void dll_free(list_t **pp_list) {
  // Functia da free unei liste

  dll_node_t *curr = (*pp_list)->head;
  dll_node_t *node = curr;
  int n = (*pp_list)->size;
  while (n) {
    node = curr;
    curr = curr->next;
    free(node->data);
    free(node);
    n--;
  }
  free(*pp_list);
  *pp_list = NULL;
}

arena_t *alloc_arena(const uint64_t size) {
  arena_t *arena = malloc(sizeof(*arena));
  if (!arena) {
    printf("malloc() failed\n");
    return NULL;
  }
  arena->arena_size = size;
  arena->used_size = 0;
  arena->alloc_list = dll_create(sizeof(block_t));
  return arena;
}

void dealloc_arena(arena_t *arena) {
  dll_node_t *node = arena->alloc_list->head;
  while (node != NULL) {
    list_t *mini = ((list_t *)((block_t *)node->data)->miniblock_list);
    dll_node_t *curr = mini->head;
    while (curr != NULL) {
      if (((miniblock_t *)curr->data)->rw_buffer != NULL)
        free(((miniblock_t *)curr->data)->rw_buffer);
      curr = curr->next;
    }
    dll_free(&mini);
    node = node->next;
  }
  dll_free(&arena->alloc_list);
  free(arena);
}

int is_node_start(arena_t *arena, const uint64_t address) {
  // Functia verifica daca exista un block in lista de block-uri care sa aiba
  // adresa de final egala cu adresa de start a noului block ce va trebui
  // adaugat in lista. Daca da, functia intoarce 1, altfel 0.

  dll_node_t *node = arena->alloc_list->head;
  while (node != NULL) {
    uint64_t adr = ((block_t *)node->data)->start_address;
    uint64_t size = ((block_t *)node->data)->size;
    if (adr + size == address) return 1;
    node = node->next;
  }
  return 0;
}

int is_node_final(arena_t *arena, const uint64_t address) {
  // Functia verifica daca exista un block in lista de block-uri care are adresa
  // de start egala cu adresa noului block c va fi adaugat.

  dll_node_t *node = arena->alloc_list->head;
  while (node != NULL) {
    if (((block_t *)node->data)->start_address == address) {
      return 1;
    }
    node = node->next;
  }
  return 0;
}

int poz(list_t *list, const uint64_t address) {
  // Functia intoarce pozitia primului block din lista care are adresa de start
  // mai mare decat adresa trimisa ca paramtetru.

  int poz = 0;
  dll_node_t *node = list->head;
  while (node != NULL) {
    if (address < ((block_t *)node->data)->start_address) {
      return poz;
    }
    node = node->next;
    poz++;
  }
  return poz;
}

dll_node_t *get_node(list_t *list, const uint64_t address) {
  // Functia intoarce nodul ce are adresa de start egala cu adresa trimisa ca
  // parametru.

  dll_node_t *node = list->head;
  while (node != NULL) {
    if (address == ((block_t *)node->data)->start_address) {
      return node;
    }
    node = node->next;
  }
  return NULL;
}

dll_node_t *get_node_poz(list_t *list, int n) {
  // Functia intoarce nodul de pe pozitia n din lista.

  dll_node_t *node = list->head;
  int poz = 0;
  while (node != NULL) {
    if (n == poz) {
      return node;
    }
    node = node->next;
    poz++;
  }
  return NULL;
}

dll_node_t *dll_remove_nth_node(list_t *list, unsigned int n) {
  // Functie de stergere a unui nod dintr-o lista dublu inlantuita.

  if (n > list->size) n = list->size;
  if (n == 0) {
    if (list->size != 1) {
      dll_node_t *first = list->head;
      list->head = first->next;
      list->head->prev = NULL;
      list->size--;
      return first;
    } else {
      dll_node_t *first = list->head;
      list->head = NULL;
      list->size--;
      return first;
    }

  } else {
    dll_node_t *node = list->head;
    dll_node_t *curr = list->head->next;
    for (unsigned int i = 0; i < n - 1; i++) {
      node = curr;
      curr = curr->next;
    }
    node->next = curr->next;
    dll_node_t *aux = curr->next;
    if (aux != NULL) {
      aux->prev = node;
    }
    curr->next = NULL;
    curr->prev = NULL;
    list->size--;
    return curr;
  }
  return 0;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size) {
  if (address >= arena->arena_size) {
    printf("The allocated address is outside the size of arena\n");
    return;
  }

  if (address + size > arena->arena_size) {
    printf("The end address is past the size of the arena\n");
    return;
  }
  if (arena->alloc_list->size != 0) {
    dll_node_t *error = arena->alloc_list->head;
    while (error != NULL && address <= ((block_t *)error->data)->start_address +
                                           ((block_t *)error->data)->size) {
      for (unsigned int i = 0; i < size; i++) {
        if (address + i >= ((block_t *)error->data)->start_address &&
            address + i < ((block_t *)error->data)->size +
                              ((block_t *)error->data)->start_address) {
          printf("This zone was already allocated.\n");
          return;
        }
      }
      error = error->next;
    }
  }

  arena->used_size += size;
  // variabila folosita pentru a avea size-ul total ocupat de block-urile
  // alocate
  uint64_t addressfinal = address + size;

  miniblock_t *miniblock = malloc(sizeof(*miniblock));
  if (!miniblock) {
    printf("malloc() failed\n");
    return;
  }

  miniblock->size = size;
  miniblock->start_address = address;
  miniblock->perm = 6;
  miniblock->rw_buffer = NULL;

  if (arena->alloc_list->size == 0 || is_node_start(arena, address) == 0) {
    int pozitie = poz(arena->alloc_list, address);
    block_t *block = malloc(sizeof(*block));
    if (!block) {
      printf("malloc() failed\n");
      return;
    }

    block->size = size;
    block->start_address = address;
    block->miniblock_list = dll_create(sizeof(miniblock_t));
    dll_add_nth_node(arena->alloc_list, pozitie, block);

    dll_add_nth_node(block->miniblock_list, 0, miniblock);

    // adresa de final a blockului nou creat
    if (is_node_final(arena, addressfinal) == 1) {
      // mutam toate block-urile care incep de la adresa finala a acestui
      // block adaugat in lista de miniblockuri a blockului nou

      dll_node_t *node = get_node(arena->alloc_list, addressfinal);

      dll_node_t *del =
          ((list_t *)((block_t *)node->data)->miniblock_list)->head;

      dll_node_t *curr = ((list_t *)block->miniblock_list)->head;
      curr->next = del;
      del->prev = curr;
      ((list_t *)block->miniblock_list)->size +=
          ((list_t *)((block_t *)node->data)->miniblock_list)->size;
      // am facut ca prev-ul primului nod din lista de miniblockuri (care
      // urmeaza sa fie deallocata) sa pointeze la nodul din noua lista de
      // miniblockuri a blockului ce tocmai a fost creat

      ((list_t *)((block_t *)node->data)->miniblock_list)->head = NULL;

      // crestem size-ul blocului curent
      curr = get_node_poz(arena->alloc_list, pozitie);
      ((block_t *)curr->data)->size += ((block_t *)node->data)->size;

      node = dll_remove_nth_node(arena->alloc_list, pozitie + 1);
      // free la blockul eliminat
      free(((list_t *)((block_t *)node->data)->miniblock_list));
      free(node->data);
      free(node);
    }
    free(block);
  } else {
    unsigned int pozitie = poz(arena->alloc_list, address) - 1;
    dll_node_t *node = get_node_poz(arena->alloc_list, pozitie);
    list_t *mini = ((list_t *)((block_t *)node->data)->miniblock_list);

    dll_add_nth_node(mini, mini->size, miniblock);
    ((block_t *)node->data)->size += size;

    if (is_node_final(arena, addressfinal) == 1) {
      dll_node_t *node2 = get_node(arena->alloc_list, addressfinal);
      dll_node_t *del =
          ((list_t *)((block_t *)node2->data)->miniblock_list)->head;

      dll_node_t *curr = mini->head;
      while (curr->next != NULL) curr = curr->next;
      // mergem pana la finalul listei de miniblockuri

      curr->next = del;
      del->prev = curr;
      mini->size += ((list_t *)((block_t *)node2->data)->miniblock_list)->size;

      ((block_t *)node->data)->size += ((block_t *)node2->data)->size;

      node = dll_remove_nth_node(arena->alloc_list, pozitie + 1);
      // free la blockul eliminat

      free(((list_t *)((block_t *)node->data)->miniblock_list));
      free(node->data);
      free(node);
    }
  }
  free(miniblock);
}

void free_block(arena_t *arena, const uint64_t address) {
  if (arena->alloc_list->size == 0) {
    printf("Invalid address for free.\n");
    return;
  }

  int pozitie = poz(arena->alloc_list, address) - 1;
  if (pozitie < 0) pozitie = 0;
  dll_node_t *node = get_node_poz(arena->alloc_list, pozitie);

  list_t *mini = ((block_t *)node->data)->miniblock_list;
  dll_node_t *curr = mini->head;
  int ok = 0;
  while (curr != NULL) {
    if (address == ((miniblock_t *)curr->data)->start_address) ok = 1;
    curr = curr->next;
  }
  if (!ok) {
    printf("Invalid address for free.\n");
    return;
  }

  dll_node_t *rm = get_node(mini, address);
  unsigned int pozmini = poz(mini, address) - 1;
  arena->used_size -= ((miniblock_t *)rm->data)->size;
  if (mini->size == 1) {
    rm = dll_remove_nth_node(mini, 0);
    free(((miniblock_t *)rm->data)->rw_buffer);
    free(rm->data);
    free(rm);
    free(mini);

    node = dll_remove_nth_node(arena->alloc_list, pozitie);
    free(node->data);
    free(node);
    return;
  }

  // verificam pe ce pozitie se afla miniblock-ul in lista de miniblock-uri.

  if (pozmini == 0) {
    rm = dll_remove_nth_node(mini, pozmini);
    ((block_t *)node->data)->size -= ((miniblock_t *)rm->data)->size;
    ((block_t *)node->data)->start_address =
        ((miniblock_t *)mini->head->data)->start_address;
    free(((miniblock_t *)rm->data)->rw_buffer);
    free(rm->data);
    free(rm);
    return;
  }
  if (pozmini == mini->size - 1) {
    rm = dll_remove_nth_node(mini, pozmini);
    ((block_t *)node->data)->size -= ((miniblock_t *)rm->data)->size;
    free(((miniblock_t *)rm->data)->rw_buffer);
    free(rm->data);
    free(rm);
    return;
  }

  // daca este primul sau ultimul din lista, lista de miniblock-uri nu se va
  // sparge( formand 2 block-uri cu 2 liste de miniblock-uri diferite).

  rm = dll_remove_nth_node(mini, pozmini);
  ((block_t *)node->data)->size -= ((miniblock_t *)rm->data)->size;
  free(((miniblock_t *)rm->data)->rw_buffer);
  free(rm->data);
  free(rm);

  curr = get_node_poz(mini, pozmini);

  uint64_t del_size = 0;
  int nrm = 0;
  while (curr != NULL) {
    del_size += ((miniblock_t *)curr->data)->size;
    nrm++;
    curr = curr->next;
  }
  ((block_t *)node->data)->size -= del_size;

  curr = get_node_poz(mini, pozmini);
  block_t *block = malloc(sizeof(*block));
  if (!block) {
    printf("malloc() failed\n");
    return;
  }

  block->start_address = ((miniblock_t *)curr->data)->start_address;
  block->size = del_size;
  block->miniblock_list = dll_create(sizeof(miniblock_t));
  dll_add_nth_node(arena->alloc_list, pozitie + 1, block);
  list_t *mini2 = (list_t *)block->miniblock_list;
  mini2->head = curr;
  node = curr->prev;
  node->next = NULL;
  curr->prev = NULL;
  mini->size -= nrm;
  mini2->size += nrm;

  // while (pozmini <= mini->size - 1) {
  //   rm = dll_remove_nth_node(mini, pozmini);
  //   dll_add_nth_node(mini2, mini2->size, rm->data);
  // }

  free(block);
}

void read(arena_t *arena, uint64_t address, uint64_t size) {
  if (arena->alloc_list->size == 0) {
    printf("Invalid address for read.\n");
    return;
  }

  dll_node_t *curr = arena->alloc_list->head;
  int ok = 0;
  while (curr != NULL) {
    if (address == ((block_t *)curr->data)->start_address) ok = 1;
    if (address > ((block_t *)curr->data)->start_address &&
        address < ((block_t *)curr->data)->start_address +
                      ((block_t *)curr->data)->size)
      ok = 1;
    curr = curr->next;
  }
  if (!ok) {
    printf("Invalid address for read.\n");
    return;
  }

  unsigned int pozitie = poz(arena->alloc_list, address) - 1;
  dll_node_t *node = get_node_poz(arena->alloc_list, pozitie);

  list_t *mini = ((block_t *)node->data)->miniblock_list;
  curr = mini->head;

  // Verificam daca toate miniblock-urile din lista au permisiuni de citire.

  while (curr != NULL) {
    ok = 0;
    if (((miniblock_t *)curr->data)->perm == 4) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 5) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 6) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 7) ok = 1;
    if (!ok) {
      printf("Invalid permissions for read.\n");
      return;
    }
    curr = curr->next;
  }

  if (((block_t *)node->data)->size < size) {
    size = ((block_t *)node->data)->size;
    printf(
        "Warning: size was bigger than the block size. Reading %lu "
        "characters.\n",
        size);
  }

  int minipoz = poz(mini, address) - 1;
  if (minipoz < 0) minipoz = 0;
  curr = get_node_poz(mini, minipoz);
  miniblock_t *miniblock = curr->data;

  if (miniblock->rw_buffer == NULL) {
    printf("Invalid address for read.\n");
    free(miniblock);
    return;
  }
  uint64_t j = ((block_t *)node->data)->start_address;
  if (address - j != 0) size++;
  uint64_t k = address - j;
  for (uint64_t i = address - j; i < size; i++) {
    if (address + i >= miniblock->size + miniblock->start_address) {
      curr = curr->next;
      miniblock = curr->data;
      k = 0;
    }
    if (((char *)miniblock->rw_buffer)[k] == 0) {
      printf("\n");
      return;
    }
    printf("%c", ((char *)miniblock->rw_buffer)[k]);
    k++;
  }
  printf("\n");
}

void write(arena_t *arena, uint64_t address, uint64_t size) {
  if (arena->alloc_list->size == 0) {
    printf("Invalid address for write.\n");
    char c;
    scanf("%c", &c);
    for (uint64_t i = 0; i < size; i++) scanf("%c", &c);
    return;
  }

  dll_node_t *curr = arena->alloc_list->head;
  int ok = 0;
  while (curr != NULL) {
    if (address == ((block_t *)curr->data)->start_address) ok = 1;
    if (address > ((block_t *)curr->data)->start_address &&
        address < ((block_t *)curr->data)->start_address +
                      ((block_t *)curr->data)->size)
      ok = 1;
    curr = curr->next;
  }
  if (!ok) {
    printf("Invalid address for write.\n");
    char c;
    scanf("%c", &c);
    for (uint64_t i = 0; i < size; i++) scanf("%c", &c);
    return;
  }

  unsigned int pozitie = poz(arena->alloc_list, address) - 1;
  dll_node_t *node = get_node_poz(arena->alloc_list, pozitie);

  list_t *mini = ((block_t *)node->data)->miniblock_list;
  curr = mini->head;

  // Verificam daca toate miniblock-urile din lista au permisiuni de scriere.
  while (curr != NULL) {
    ok = 0;
    if (((miniblock_t *)curr->data)->perm == 2) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 3) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 6) ok = 1;
    if (((miniblock_t *)curr->data)->perm == 7) ok = 1;
    if (!ok) {
      printf("Invalid permissions for write.\n");
      char c;
      scanf("%c", &c);
      for (uint64_t i = 0; i < size; i++) scanf("%c", &c);
      return;
    }
    curr = curr->next;
  }

  int old_size = 0;
  if (((block_t *)node->data)->size < size) {
    old_size = size;
    size = ((block_t *)node->data)->size;
    printf(
        "Warning: size was bigger than the block size. Writing %lu "
        "characters.\n",
        size);
  }

  unsigned int minipoz = poz(mini, address) - 1;
  curr = get_node_poz(mini, minipoz);

  miniblock_t *miniblock = curr->data;
  char c;

  // alocam buffer-ul in care scriem cu calloc pentru a fi plin de 0-uri(pentru
  // verificari este util)

  miniblock->rw_buffer = calloc(miniblock->size, sizeof(char));
  if (!miniblock->rw_buffer) {
    printf("calloc() failed\n");
    return;
  }

  scanf("%c", &c);
  uint64_t j = ((block_t *)node->data)->start_address;
  uint64_t k = address - j;
  for (uint64_t i = address - j; i < size; i++) {
    if (address + i >= miniblock->size + miniblock->start_address) {
      curr = curr->next;
      miniblock = curr->data;
      miniblock->rw_buffer = calloc(miniblock->size, sizeof(char));
      k = 0;
      if (!miniblock->rw_buffer) {
        printf("calloc() failed\n");
        return;
      }
    }

    scanf("%c", &c);
    ((char *)miniblock->rw_buffer)[k] = c;
    k++;
  }
  // daca nu s-au putut scrie toate caracterele, citim restul de caractere ce
  // nu au putut sa fie scrise in miniblock-uri.
  if (old_size != 0) {
    old_size -= size;
    for (int i = 0; i < old_size; i++) scanf("%c", &c);
  }
}

void pmap(const arena_t *arena) {
  printf("Total memory: 0x%lX bytes\n", arena->arena_size);

  printf("Free memory: 0x%lX bytes\n", arena->arena_size - arena->used_size);

  printf("Number of allocated blocks: %d\n", arena->alloc_list->size);

  dll_node_t *curr = arena->alloc_list->head;
  int nrm = 0;
  while (curr != NULL) {
    int n = ((list_t *)((block_t *)curr->data)->miniblock_list)->size;
    nrm += n;
    curr = curr->next;
  }

  printf("Number of allocated miniblocks: %d\n", nrm);

  dll_node_t *node = arena->alloc_list->head;
  int nr = 0;
  while (node != NULL) {
    nr++;
    printf("\n");
    printf("Block %d begin\n", nr);

    uint64_t addr1 = ((block_t *)node->data)->start_address;
    uint64_t size = ((block_t *)node->data)->size;
    uint64_t addr2 = addr1 + size;
    printf("Zone: 0x%lX - 0x%lX\n", addr1, addr2);

    int n = ((list_t *)((block_t *)node->data)->miniblock_list)->size;
    dll_node_t *curr =
        ((list_t *)((block_t *)node->data)->miniblock_list)->head;
    for (int i = 1; i <= n; i++) {
      printf("Miniblock %d:\t\t", i);
      uint64_t addrmini1 = ((miniblock_t *)curr->data)->start_address;
      uint64_t sizemini = ((miniblock_t *)curr->data)->size;
      uint64_t addrmini2 = addrmini1 + sizemini;
      printf("0x%lX\t\t-\t\t0x%lX\t\t|", addrmini1, addrmini2);

      if (((miniblock_t *)curr->data)->perm == 6) printf(" RW-\n");
      if (((miniblock_t *)curr->data)->perm == 0) printf(" ---\n");
      if (((miniblock_t *)curr->data)->perm == 2) printf(" -W-\n");
      if (((miniblock_t *)curr->data)->perm == 3) printf(" -WX\n");
      if (((miniblock_t *)curr->data)->perm == 7) printf(" RWX\n");
      if (((miniblock_t *)curr->data)->perm == 4) printf(" R--\n");
      if (((miniblock_t *)curr->data)->perm == 1) printf(" --X\n");
      if (((miniblock_t *)curr->data)->perm == 5) printf(" R-X\n");
      curr = curr->next;
    }

    printf("Block %d end\n", nr);
    node = node->next;
  }
}
void mprotect(arena_t *arena, uint64_t address) {
  char p[STRING_MAX];
  int perm = 0;

  if (arena->alloc_list->size == 0) {
    printf("Invalid address for mprotect.\n");
    fgets(p, STRING_MAX, stdin);
    return;
  }

  int pozitie = poz(arena->alloc_list, address) - 1;
  if (pozitie < 0) pozitie = 0;
  dll_node_t *node = get_node_poz(arena->alloc_list, pozitie);
  list_t *mini = ((block_t *)node->data)->miniblock_list;
  dll_node_t *curr = mini->head;
  int ok = 0;
  while (curr != NULL) {
    if (address == ((miniblock_t *)curr->data)->start_address) ok = 1;
    curr = curr->next;
  }

  if (!ok) {
    printf("Invalid address for mprotect.\n");
    fgets(p, STRING_MAX, stdin);
    return;
  }

  fgets(p, STRING_MAX, stdin);
  char *t = strtok(p, " \n");
  while (t != NULL) {
    if (strcmp(t, "PROT_NONE") == 0) perm += 0;
    if (strcmp(t, "PROT_READ") == 0) perm += 4;
    if (strcmp(t, "PROT_EXEC") == 0) perm += 1;
    if (strcmp(t, "PROT_WRITE") == 0) perm += 2;
    t = strtok(NULL, " \n");
  }

  curr = get_node(mini, address);
  ((miniblock_t *)curr->data)->perm = perm;
}