LDADD = \
	../libgiggle/libgiggle.la \
	../libgiggle-git/libgiggle-git.la \
	$(GIGGLE_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(GIGGLE_CFLAGS) \
	$(NULL)

check_PROGRAMS = \
	check-bare

TESTS = $(check_PROGRAMS)

EXTRA_DIST = \
	multi-root.git/index \
	multi-root.git/refs/heads/master \
	multi-root.git/refs/heads/import-other \
	multi-root.git/refs/heads/temp \
	multi-root.git/logs/refs/heads/master \
	multi-root.git/logs/refs/heads/import-other \
	multi-root.git/logs/refs/heads/temp \
	multi-root.git/logs/HEAD \
	multi-root.git/objects/cd/d30666ad062e52db78590fd67ef9b2ff1ad0f8 \
	multi-root.git/objects/ff/c7dc4200ee1db33fc8de7f6e0fed284800513c \
	multi-root.git/objects/22/fe0e9ac9fc805143b1071e477525a30ece2954 \
	multi-root.git/objects/ae/c24f289e269b53ca40a2d396f4a20e91444772 \
	multi-root.git/objects/e6/9de29bb2d1d6434b8b29ae775ad8c2e48c5391 \
	multi-root.git/objects/ea/5b0d644c3efb59854307544f20b3f0b78d70b3 \
	multi-root.git/objects/19/0e8ecbd54f96de931016366f8148a57913e8fa \
	multi-root.git/objects/42/1c7da988edc35f32bec1e94d0ebdbc2aa7d709 \
	multi-root.git/objects/ef/d37f3fc9c492d901a379c710d50931d9a0881e \
	multi-root.git/FETCH_HEAD \
	multi-root.git/info/exclude \
	multi-root.git/description \
	multi-root.git/HEAD \
	multi-root.git/ORIG_HEAD \
	multi-root.git/config

