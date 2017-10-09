#ifndef _K_COMPILER_H_
#define _K_COMPILER_H_ 1

#define BUILD_BUG_ON(condition) extern int build_bug_on[!!!(condition)]
#define BUILD_BUG_ON_FALSE(condition) extern int build_bug_on[!!(condition)]
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:(-!!(e)); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:(-!!(e)); }))

#endif
