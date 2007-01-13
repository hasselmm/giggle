#include <gtk/gtk.h>

#ifndef __GIG_REVISION_INFO_H__
#define __GIG_REVISION_INFO_H__

G_BEGIN_DECLS

typedef enum GigRevisionType GigRevisionType;
typedef struct GigRevisionInfo GigRevisionInfo;
typedef struct GigBranchInfo GigBranchInfo;

struct GigBranchInfo {
	gchar *name;
};

enum GigRevisionType {
	GIG_REVISION_BRANCH,
	GIG_REVISION_MERGE,
	GIG_REVISION_COMMIT
};

struct GigRevisionInfo {
	GigRevisionType type;
	GigBranchInfo *branch1; /* only this one will be used in GIG_REVISION_COMMIT */
	GigBranchInfo *branch2;

	/* stuff that will be filled out
	 * in the validation process
	 */
	GHashTable *branches;
};

GigBranchInfo   *gig_branch_info_new  (const gchar *name);
void             gig_branch_info_free (GigBranchInfo *info);

GigRevisionInfo *gig_revision_info_new_commit (GigBranchInfo *branch);
GigRevisionInfo *gig_revision_info_new_branch (GigBranchInfo *old, GigBranchInfo *new);
GigRevisionInfo *gig_revision_info_new_merge  (GigBranchInfo *to, GigBranchInfo *from);
void             gig_revision_info_free       (GigRevisionInfo *info);

void  gig_revision_info_validate (GtkTreeModel *model, gint n_column);

G_END_DECLS

#endif /* __GIG_REVISION_INFO_H__ */
