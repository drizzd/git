#include <assert.h>
#include "git-compat-util.h"
#include "cache.h"
#include "tree-walk.h"

static const char *usage_msg = "test-traverse-index";

struct tree_entry {
	unsigned char sha1[20];
	const char *path;
};

struct tree_entry_list {
	struct tree_entry *entry;
	int len;
};

#define DIV_CEIL(num, den) (((num)+(den)-1)/(den))

static struct cache_entry *create_cache_entry(const struct tree_entry *entry)
{
	int mode = 0100644;
	struct cache_entry *ce;
	int namelen = strlen(entry->path);
	unsigned size = cache_entry_size(namelen);

	ce = xcalloc(1, size);
	hashcpy(ce->sha1, entry->sha1);
	memcpy(ce->name, entry->path, namelen);
	ce->ce_mode = create_ce_mode(mode);
	ce->ce_flags = create_ce_flags(0);
	ce->ce_namelen = namelen;

	return ce;
}

static struct index_state *create_index(const struct tree_entry_list *sample)
{
	int i;
	struct index_state *index;

	index = xcalloc(1, sizeof(*index));

	for (i = 0; i < sample->len; i++) {
		struct cache_entry *ce;
		ce = create_cache_entry(&sample->entry[i]);
		if (add_index_entry(index, ce, ADD_CACHE_OK_TO_ADD) < 0)
			die(_("unable to add cache entry: %s"), ce->name);
	}

	return index;
}

static void test_index(const struct tree_entry_list *sample)
{
	struct index_state *index;

	index = create_index(sample);

	discard_index(index);
	free(index);
}

static void create_tree(struct strbuf *treebuf, const struct tree_entry_list *sample)
{
	int i;
	unsigned int mode = 0100644;

	for (i = 0; i < sample->len; i++) {
		struct tree_entry *entry = &sample->entry[i];
		strbuf_addf(treebuf, "%o %s%c", mode, entry->path, '\0');
		strbuf_add(treebuf, entry->sha1, 20);
	}
}

#define OBJ_NR_SIZE sizeof(((struct tree_entry *)NULL)->obj_nr)

static void test_traverse_tree(const struct tree_entry_list *sample,
		char *buffer, int len)
{
	int i;
	struct tree_desc desc;

	init_tree_desc(&desc, buffer, len);
	for (i = 0; i < sample->len; i++) {
		struct tree_entry *entry = &sample->entry[i];

		assert(desc.size);
		assert(!hashcmp(desc.entry.sha1, entry->sha1));
		assert(!strcmp(desc.entry.path, entry->path));

		update_tree_entry(&desc);
	}

	/* end of tree */
	assert(!desc.size);
}

static void test_tree(const struct tree_entry_list *sample)
{
	struct strbuf treebuf = STRBUF_INIT;

	create_tree(&treebuf, sample);
	test_traverse_tree(sample, treebuf.buf, treebuf.len);

	strbuf_release(&treebuf);
}

struct tree_entry_spec {
	unsigned int obj_nr;
	const char *path;
};

static const unsigned char *sha1_from_obj_nr(unsigned int obj_nr)
{
	int i;
	static unsigned char sha1[20];

	for (i = 0; i < ARRAY_SIZE(sha1); i++)
		sha1[i] = 0;
	for (i = 0; i < sizeof(obj_nr); i++)
		sha1[i] = (obj_nr >> (i * CHAR_BIT)) & ~(1 << CHAR_BIT);

	return sha1;
}

static void tree_entry_init(struct tree_entry *entry, struct tree_entry_spec *spec)
{
	entry->path = spec->path;
	hashcpy(entry->sha1, sha1_from_obj_nr(spec->obj_nr));
}

static void tree_entry_list_init(struct tree_entry_list *list, struct tree_entry_spec *spec, int len)
{
	int i;

	list->len = len;
	list->entry = xmalloc(len * sizeof(*list->entry));
	for (i = 0; i < len; i++)
		tree_entry_init(&list->entry[i], &spec[i]);
}

static void tree_entry_list_free(struct tree_entry_list *list)
{
	free(list->entry);
	list->entry = NULL;
	list->len = 0;
}

static void all_tests(void)
{
	struct tree_entry_list sample;
	struct tree_entry_spec sample_spec[] = {
		{ 1, "a"},
		{ 2, "c"},
		{ 3, "b"}
	};

	tree_entry_list_init(&sample, sample_spec, ARRAY_SIZE(sample_spec));

	test_index(&sample);
	test_tree(&sample);

	tree_entry_list_free(&sample);
}

int main(int argc, char **argv)
{
	if (argc > 1)
		usage(usage_msg);

	all_tests();

	return 0;
}
