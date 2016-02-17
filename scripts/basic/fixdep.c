#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <arpa/inet.h>

#define INT_CONF ntohl(0x434f4e46)
#define INT_ONFI ntohl(0x4f4e4649)
#define INT_NFIG ntohl(0x4e464947)
#define INT_FIG_ ntohl(0x4649475f)

char *target;
char *depfile;
char *cmdline;

static void usage(void)
{
	fprintf(stderr, "Usage: fixdep <depfile> <target> <cmdline>\n");
	exit(1);
}

/*
 * Print out the commandline prefixed with cmd_<target filename> :=
 */
static void print_cmdline(void)
{
	printf("cmd_%s := %s\n\n", target, cmdline);
}

struct item {
	struct item	*next;
	unsigned int	len;
	unsigned int	hash;
	char		name[0];
};

#define HASHSZ 256
static struct item *hashtab[HASHSZ];

static unsigned int strhash(const char *str, unsigned int sz)
{
	/* fnv32 hash */
	unsigned int i, hash = 2166136261U;

	for (i = 0; i < sz; i++)
		hash = (hash ^ str[i]) * 0x01000193;
	return hash;
}

/*
 * Lookup a value in the configuration string.
 */
static int is_defined_config(const char *name, int len, unsigned int hash)
{
	struct item *aux;

	for (aux = hashtab[hash % HASHSZ]; aux; aux = aux->next) {
		if (aux->hash == hash && aux->len == len &&
		    memcmp(aux->name, name, len) == 0)
			return 1;
	}
	return 0;
}

/*
 * Add a new value to the configuration string.
 */
static void define_config(const char *name, int len, unsigned int hash)
{
	struct item *aux = malloc(sizeof(*aux) + len);

	if (!aux) {
		perror("fixdep:malloc");
		exit(1);
	}
	memcpy(aux->name, name, len);
	aux->len = len;
	aux->hash = hash;
	aux->next = hashtab[hash % HASHSZ];
	hashtab[hash % HASHSZ] = aux;
}

/*
 * Clear the set of configuration strings.
 */
static void clear_config(void)
{
	struct item *aux, *next;
	unsigned int i;

	for (i = 0; i < HASHSZ; i++) {
		for (aux = hashtab[i]; aux; aux = next) {
			next = aux->next;
			free(aux);
		}
		hashtab[i] = NULL;
	}
}

/*
 * Record the use of a CONFIG_* word.
 */
static void use_config(const char *m, int slen)
{
	unsigned int hash = strhash(m, slen);
	int c, i;

	if (is_defined_config(m, slen, hash))
	    return;

	define_config(m, slen, hash);

	printf("    $(wildcard include/config/");
	for (i = 0; i < slen; i++) {
		c = m[i];
		if (c == '_')
			c = '/';
		else
			c = tolower(c);
		putchar(c);
	}
	printf(".h) \\\n");
}

static void parse_config_file(const char *map, size_t len)
{
	const int *end = (const int *) (map + len);
	/* start at +1, so that p can never be < map */
	const int *m   = (const int *) map + 1;
	const char *p, *q;

	for (; m < end; m++) {
		if (*m == INT_CONF) { p = (char *) m  ; goto conf; }
		if (*m == INT_ONFI) { p = (char *) m-1; goto conf; }
		if (*m == INT_NFIG) { p = (char *) m-2; goto conf; }
		if (*m == INT_FIG_) { p = (char *) m-3; goto conf; }
		continue;
	conf:
		if (p > map + len - 7)
			continue;
		if (memcmp(p, "CONFIG_", 7))
			continue;
		for (q = p + 7; q < map + len; q++) {
			if (!(isalnum(*q) || *q == '_'))
				goto found;
		}
		continue;

	found:
		if (!memcmp(q - 7, "_MODULE", 7))
			q -= 7;
		if( (q-p-7) < 0 )
			continue;
		use_config(p+7, q-p-7);
	}
}

/* test is s ends in sub */
static int strrcmp(char *s, char *sub)
{
	int slen = strlen(s);
	int sublen = strlen(sub);

	if (sublen > slen)
		return 1;

	return memcmp(s + slen - sublen, sub, sublen);
}

static void do_config_file(const char *filename)
{
	struct stat st;
	int fd;
	void *map;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "fixdep: error opening config file: ");
		perror(filename);
		exit(2);
	}
	fstat(fd, &st);
	if (st.st_size == 0) {
		close(fd);
		return;
	}
	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if ((long) map == -1) {
		perror("fixdep: mmap");
		close(fd);
		return;
	}

	parse_config_file(map, st.st_size);

	munmap(map, st.st_size);

	close(fd);
}

/*
 * Important: The below generated source_foo.o and deps_foo.o variable
 * assignments are parsed not only by make, but also by the rather simple
 * parser in scripts/mod/sumversion.c.
 */
static void parse_dep_file(void *map, size_t len)
{
	char *m = map;
	char *end = m + len;
	char *p;
	char s[PATH_MAX];
	int is_target;
	int saw_any_target = 0;
	int is_first_dep = 0;

	clear_config();

	while (m < end) {
		/* Skip any "white space" */
		while (m < end && (*m == ' ' || *m == '\\' || *m == '\n'))
			m++;
		/* Find next "white space" */
		p = m;
		while (p < end && *p != ' ' && *p != '\\' && *p != '\n')
			p++;
		/* Is the token we found a target name? */
		is_target = (*(p-1) == ':');
		/* Don't write any target names into the dependency file */
		if (is_target) {
			/* The /next/ file is the first dependency */
			is_first_dep = 1;
		} else {
			/* Save this token/filename */
			memcpy(s, m, p-m);
			s[p - m] = 0;

			/* Ignore certain dependencies */
			if (strrcmp(s, "include/generated/autoconf.h") &&
			    strrcmp(s, "arch/um/include/uml-config.h") &&
			    strrcmp(s, "include/linux/kconfig.h") &&
			    strrcmp(s, ".ver")) {
				/*
				 * Do not list the source file as dependency,
				 * so that kbuild is not confused if a .c file
				 * is rewritten into .S or vice versa. Storing
				 * it in source_* is needed for modpost to
				 * compute srcversions.
				 */
				if (is_first_dep) {
					/*
					 * If processing the concatenation of
					 * multiple dependency files, only
					 * process the first target name, which
					 * will be the original source name,
					 * and ignore any other target names,
					 * which will be intermediate temporary
					 * files.
					 */
					if (!saw_any_target) {
						saw_any_target = 1;
						printf("source_%s := %s\n\n",
							target, s);
						printf("deps_%s := \\\n",
							target);
					}
					is_first_dep = 0;
				} else
					printf("  %s \\\n", s);
				do_config_file(s);
			}
		}
		/*
		 * Start searching for next token immediately after the first
		 * "whitespace" character that follows this token.
		 */
		m = p + 1;
	}

	if (!saw_any_target) {
		fprintf(stderr, "fixdep: parse error; no targets found\n");
		exit(1);
	}

	printf("\n%s: $(deps_%s)\n\n", target, target);
	printf("$(deps_%s):\n", target);
}

static void print_deps(void)
{
	struct stat st;
	int fd;
	void *map;

	fd = open(depfile, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "fixdep: error opening depfile: ");
		perror(depfile);
		exit(2);
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "fixdep: error fstat'ing depfile: ");
		perror(depfile);
		exit(2);
	}
	if (st.st_size == 0) {
		fprintf(stderr,"fixdep: %s is empty\n",depfile);
		close(fd);
		return;
	}
	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if ((long) map == -1) {
		perror("fixdep: mmap");
		close(fd);
		return;
	}

	parse_dep_file(map, st.st_size);

	munmap(map, st.st_size);

	close(fd);
}

static void traps(void)
{
	static char test[] __attribute__((aligned(sizeof(int)))) = "CONF";
	int *p = (int *)test;

	if (*p != INT_CONF) {
		fprintf(stderr, "fixdep: sizeof(int) != 4 or wrong endianness? %#x\n",
			*p);
		exit(2);
	}
}

int main(int argc, char *argv[])
{
	traps();

	if (argc != 4)
		usage();

	depfile = argv[1];
	target = argv[2];
	cmdline = argv[3];

	print_cmdline();
	print_deps();

	return 0;
}
