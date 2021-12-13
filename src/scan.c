#include "scan.h"
static char *NL = "\x0a";
static char *WL = "wl.txt";
static char *SITES = "sites.txt";
static pthread_mutex_t printf_mutex;
thread_path* plist_thread = NULL;

char *
__strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);
  if (new == NULL)
    return NULL;
  return (char *) memcpy (new, s, len);
}
// Compare function to trim trailing slash in url
bool TrimSlash(int c) {
  return (c == '/') ;
}

char *ltrim(char *s, bool (*cmpfn)(int))
{
    while(cmpfn(*s)) s++;
    return s;
}

char *rtrim(char *s, bool (*cmpfn)(int))
{
    char* back = s + strlen(s);
    while(cmpfn(*--back));
    *(back+1) = '\0';
    return s;
}

static void dump(const void *data, size_t size) {
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    printf("%02X ", ((unsigned char *)data)[i]);
    if (((unsigned char *)data)[i] >= ' ' &&
        ((unsigned char *)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char *)data)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      printf(" ");
      if ((i + 1) % 16 == 0) {
        printf("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          printf(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          printf("   ");
        }
        printf("|  %s \n", ascii);
      }
    }
  }
}

int sync_printf(const char *format, ...) {
    va_list args;
    int done;
    va_start(args, format);

    pthread_mutex_lock(&printf_mutex);
    done = vprintf(format, args);
    pthread_mutex_unlock(&printf_mutex);

    va_end(args);
    return done;
}

static size_t MemCallBack(void *contents, size_t size, size_t nmemb,
                          void *userp) {
  return size * nmemb;
}

static unsigned long int get_stcode(char *url) {
  CURL *curl;
  CURLcode result;
  unsigned long int st_code;
  struct curl_slist *header = NULL;
  curl = curl_easy_init();
  if (curl) {
    header = curl_slist_append(header, "x-requested-with: XMLHttpRequest");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, MemCallBack);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FF/1.0");
    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
      fprintf(stderr, "curl error: %s\n", curl_easy_strerror(result));
      curl_easy_cleanup(curl);
      return -1;
    }
    result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &st_code);
    if (result != CURLE_OK) {
      fprintf(stderr, "curl_easy_getinfo error: %s\n",
              curl_easy_strerror(result));
      curl_slist_free_all(header);
      curl_easy_cleanup(curl);
      return -1;
    }
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);
    return st_code;
  }
  return -1;
}

static char *gen_dirs(const char* fname) {
  FILE *f;
  size_t nread;
  long int size;
  char *buf = NULL;
  if( access( fname, F_OK ) != 0 ) {
    fprintf(stderr, "File doesn't exist - %s\n", fname);
    return NULL;
  }
  f = fopen(fname, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(size + 1);
    nread = fread(buf, 1, size, f);
    fclose(f);
  } else {
    fprintf(stderr, "Error opening file: %s\n", strerror(errno));
    return NULL;
  }
  if(!nread) {
    fprintf(stderr, "%s file is empty\n", fname);
    free(buf);
    return NULL;
  }
  buf[size] = '\0';
  return buf;
}

static void parse_buff(char *list, 
    void *args,
    void(*insert_callback)(char*, size_t, void*)) {
  char *path;
  size_t ii = 0;
  path = strtok(list, NL);
  while (path != NULL) {
    insert_callback(path, ii, args);
    path = strtok(NULL, NL);
    ii++;
  }
  free(list);
}

void harvest_url(char *url, char *old_path) {
  size_t total_size;
  char *full_url;
  unsigned long int st_code;
  char *path = ltrim(old_path, TrimSlash);
  total_size = strlen(url) + strlen(path) + 1;
  full_url = malloc(total_size);
  snprintf(full_url, total_size, "%s%s", url, path);
  st_code = get_stcode(full_url);
  sync_printf("%s | response code = %lu\n",full_url, st_code);
  free(full_url);
}

#define pop_node(path_list, node) node = list_head(path_list);
#define del_node(path_list, node) deleteNode(path_list, node);

struct thread_info {
  pthread_t t_id;
  size_t t_num;
  size_t path_cnt;
  size_t curr_index;
  size_t block_size;
  char* url;
} typedef thread_info;


void* start_thread(void *arg) {
  thread_info* t_info = (thread_info*)arg;
#if 0
  sync_printf("\t Thread started %ld " 
      "block size %ld " 
      "curr_index %ld\n", t_info->t_num, t_info->block_size, t_info->curr_index);
#endif
  for(int i = 0; i < t_info->block_size; i ++) {
    harvest_url(t_info->url, plist_thread[t_info->curr_index + i].path);
  }
  return NULL;
}

int do_scan(char* url, size_t path_cnt) {
  size_t min_per_thread = 10;
  size_t max_per_thread = (path_cnt + min_per_thread-1) / min_per_thread;
  int p_count = get_nprocs();
  size_t thread_cnt = MIN(((p_count > 1) ? p_count: 2), max_per_thread);
  size_t block_size = path_cnt / thread_cnt;
  size_t old_block_size = block_size;
  size_t remainder = path_cnt - block_size * thread_cnt;
  int pp = 0; // passed path count
  
  thread_info* t_list = calloc(thread_cnt, sizeof(struct thread_info));
  for(size_t i = 0; i < thread_cnt; i ++) {
    t_list[i].t_num = i;
    t_list[i].block_size = block_size;
    t_list[i].curr_index= pp;
    t_list[i].path_cnt = path_cnt;
    t_list[i].url = url;
    pthread_create(&t_list[i].t_id, NULL, &start_thread, &t_list[i]);
    if(remainder) {
      block_size+=remainder;
      remainder = 0;
    } else {
      block_size = old_block_size;
    }
    pp += block_size;
  }
  for(size_t i = 0; i < thread_cnt; i ++) {
    int s = pthread_join(t_list[i].t_id, NULL);
  }
  free(t_list);
  return 0;
}

void insert_url(char* url, size_t entry, void* args) {
  /* if you want to do antything on url string, 
   * you can do that here*/
  if(args) {
    List* list = (List*) args;
    char *new_url;
    const char slash = '/';
    size_t len = strlen(url);
    if(url[len - 1] != slash) {
      new_url = malloc(len + 2);
      memcpy(new_url, url, len);
      memcpy(new_url + len, &slash, 1);
      new_url[len + 1] = '\x00';
    } else {
      new_url = malloc(len + 1);
      memcpy(new_url, url, len + 1);
    }
    addNodeTail(list, new_url);
  }
}

void insert_path(char* path, size_t entry, 
    void* args /* useless argument in this case */) {
  /* if you want to do antything on path string, you can here*/
  plist_thread[entry].path = __strdup(path);
}

size_t calc_cnt(char* buff) {
  size_t lines = 0;
  while(*buff) {
    char ch = *buff;
    if(ch == '\n') lines++;
    buff++;
  }
  return lines;
}

/* Create URL and PATH list */
size_t create_url_path(List* url_res, char* url_buff, char* path_buff) {
  /* allocate memory for path list that will be used in THREADs*/
  size_t path_cnt = calc_cnt(path_buff);
  plist_thread = calloc(path_cnt, sizeof(thread_path));
  parse_buff(url_buff, url_res, insert_url);
  parse_buff(path_buff, NULL, insert_path);
  return path_cnt;
}

int main(int argc, char *argv[]) {
  List *url_list;
  Node* node;
  char *curr_wl = WL;
  char *curr_sites = SITES;
  for(int j = 1; j < argc; j ++ ) {
    if(!strcmp(argv[j], "--words")) {
      curr_wl = argv[++j];
    }
    else if(!strcmp(argv[j], "--sites")) {
      curr_sites = argv[++j];
    }
    else {
      printf(
        "Usage: %s [--words <wordlist>] [--sites <siteslist>] \n",argv[0]);
        exit(1);
    }
  }
  curl_global_init(CURL_GLOBAL_DEFAULT);
  char *list_buff = gen_dirs(curr_wl);
  char *url_buff = gen_dirs(curr_sites);
  if(list_buff == NULL || url_buff == NULL) exit(0);
  pthread_mutex_init(&printf_mutex, NULL);
  url_list = createList(1, NULL);
  size_t path_cnt = create_url_path(url_list, url_buff, list_buff); 
  list_for_each_entry(url_list, node) {
    do_scan(node->ptr, path_cnt);
  }
  if(plist_thread) {
    for(int i = 0; i < path_cnt; i ++) {
      free(plist_thread[i].path);
    }
    free(plist_thread);
  }
  totaldealloc(url_list);
  return 0;
}
