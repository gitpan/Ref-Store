#ifndef HREG_H_
#define HREG_H_

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <stdint.h>

//#define HR_DEBUG

#ifndef HR_DEBUG
static int _hr_enable_debug = -1;
#define _hr_can_debug \
    (_hr_enable_debug >= 0 ? _hr_enable_debug : \
        (_hr_enable_debug = getenv("HR_DEBUG") ? 1 : 0))

#define HR_DEBUG(fmt, ...) if(_hr_can_debug) { \
    fprintf(stderr, "[%s:%d (%s)] " fmt "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__); \
}
#endif

#define _hashref_eq(r1, r2) \
    (SvROK(r1) && SvROK(r2) && \
    SvRV(r1) == SvRV(r2))

#define _xv_deref_complex_ok(ref, target, to) (SvROK(ref) && SvTYPE(target=(to*)SvRV(ref)) == SVt_PV ## to)

#define _sv_deref_mg_ok(ref, target) (SvROK(ref) && SvTYPE(target=SvRV(ref)) == SVt_PVMG)

#define _mg_action_list(mg) (HR_Action*)mg->mg_ptr

#define mk_ptr_string(vname, ptr) \
    char vname[128] = { '\0' }; \
    sprintf(vname, "%lu", ptr);


//#define HR_MAKE_PARENT_RV

#define HREG_API_INTERNAL

typedef enum {
    HR_ACTION_TYPE_NULL         = 0,
    HR_ACTION_TYPE_DEL_AV       = 1,
    HR_ACTION_TYPE_DEL_HV       = 2,
    HR_ACTION_TYPE_CALL_CV      = 3,
    HR_ACTION_TYPE_CALL_CFUNC   = 4,
} HR_ActionType_t;

typedef enum {
    HR_KEY_TYPE_NULL            = 0,
    HR_KEY_TYPE_PTR             = 1,
    HR_KEY_TYPE_STR             = 2,
    
    /*Extended options for searching*/
    
    /*RV implies we should:
     1) check the flags to see if the stored key is an RV,
     2) compare the keys performing SvRV on the stored key,
        assume current search spec is already dereferenced
    */
    HR_KEY_STYPE_PTR_RV          = 100 
    
} HR_KeyType_t;

typedef enum {
    HR_ACTION_NOT_FOUND,
    HR_ACTION_DELETED,
    HR_ACTION_EMPTY
} HR_DeletionStatus_t;


enum {
    HR_FLAG_STR_NO_ALLOC        = 1 << 0, //Do not copy/allocate/free string
    HR_FLAG_HASHREF_WEAKEN      = 1 << 1, //not really used
    HR_FLAG_SV_REFCNT_DEC       = 1 << 2, //Key is an SV whose REFCNT we should decrease
    HR_FLAG_PTR_NO_STRINGIFY    = 1 << 3, //Do not stringify the pointer    
};

/*We re-use the STR_NO_ALLOC field for an SV flag, which is obviously a TYPE_PTR*/
#define HR_FLAG_SV_KEY (1<<0)

#define action_key_is_rv(aptr) ((aptr)->flags & HR_FLAG_SV_REFCNT_DEC)
#define action_container_is_sv(aptr) ((aptr->atype != HR_ACTION_TYPE_CALL_CFUNC))

typedef struct HR_Action HR_Action;
typedef void(*HR_ActionCallback)(void*,SV*);

struct
__attribute__((__packed__))
HR_Action {
    HR_Action   *next;
    void        *key;
    
    //Action type and union
    unsigned int atype : 3;
    
    //TODO: Implement the unions
    
    //union {
    //    SV *hashref;
    //    SV *arrayref;
    //    HR_ActionCallback *cb;
    //} u_action;
    
    //Key type and union
    unsigned int ktype : 2;
    
    //union {
    //    unsigned int idx;
    //    void *ptr;
    //    char *key;
    //} u_selector;
    
    SV          *hashref;
    
    unsigned int flags : 5;
    /*TODO:
     instead of just using a hashref, specify an action type, perhaps deleting
     something from an arrayref or calling a subroutine directly
    */
};

#define HR_ACTION_LIST_TERMINATOR \
{ NULL, NULL, HR_KEY_TYPE_NULL, HR_ACTION_TYPE_NULL, 0, 0 }

/*Helper macros for common HR_Action specifications*/
#define HR_DREF_FLDS_ptr_from_hv(ptr, container) \
    { .ktype = HR_KEY_TYPE_PTR, .atype = HR_ACTION_TYPE_DEL_HV, \
      .key = (char*)(ptr), .hashref = container }

#define HR_DREF_FLDS_Nstr_from_hv(newstr, container) \
    { .ktype = HR_KEY_TYPE_STR, .atype = HR_ACTION_TYPE_DEL_HV, \
        .key = newstr, .hashref = container }

#define HR_DREF_FLDS_Estr_from_hv(estr, container) \
    { .ktype = HR_KEY_TYPE_STR, .atype = HR_ACTION_TYPE_DEL_HV, \
    .key = estr, .hashref = container, .flags = HR_FLAG_STR_NO_ALLOC }

#define HR_DREF_FLDS_arg_for_cfunc(arg, fptr) \
    { .ktype = HR_KEY_TYPE_PTR, .atype = HR_ACTION_TYPE_CALL_CFUNC, \
    .key = arg, .hashref = (SV*)fptr }

HREG_API_INTERNAL
void HR_add_action(HR_Action *action_list, HR_Action *new_action, int want_unique);

HREG_API_INTERNAL
void HR_trigger_and_free_actions(HR_Action *action_list, SV *object);

HREG_API_INTERNAL
HR_DeletionStatus_t
HR_del_action(HR_Action *action_list, SV *hashref, void *key, HR_KeyType_t ktype);

HREG_API_INTERNAL
HR_DeletionStatus_t
HR_nullify_action(HR_Action *action_list, SV *hashref, void *key, HR_KeyType_t ktype);

HREG_API_INTERNAL
HR_Action*
HR_free_action(HR_Action *action);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// Perl Functions                                                           ///
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
HREG_API_INTERNAL
void HR_add_actions_real(SV *objref, HR_Action *actions);

/*Perl API*/

void HR_PL_add_action_ptr(SV *objref, SV *hashref);
void HR_PL_add_action_str(SV *objref, SV *hashref, char *key);

void HR_PL_del_action_ptr(SV *object, SV *hashref, UV addr);
void HR_PL_del_action_str(SV *object, SV *hashref, char *str);
void HR_PL_del_action_container(SV *object, SV *hashref);
void HR_PL_del_action_sv(SV *object, SV *hashref, SV *keysv);

/* H::R implementation */

SV*  HRXSK_new(char *package, char *key, SV *forward, SV *scalar_lookup);
char *HRXSK_kstring(SV* self);
void HRXSK_ithread_postdup(SV *newself, SV *newtable, HV *ptr_map, UV old_table);



SV* HRXSK_encap_new(char *package, SV *encapsulated_object,
                    SV *table, SV *forward, SV *scalar_lookup);

UV HRXSK_encap_kstring(SV *ksv_ref);
void HRXSK_encap_weaken(SV *ksv_ref);
void HRXSK_encap_link_value(SV *self, SV *value);
SV*  HRXSK_encap_getencap(SV *self);
void HRXSATTR_unlink_value(SV *aobj, SV *value);
SV*  HRXSATTR_get_hash(SV *aobj);
char* HRXSATTR_kstring(SV *aobj);

void HRXSK_encap_ithread_predup(SV *self, SV *table, HV *ptr_map, SV *value);
void HRXSK_encap_ithread_postdup(SV *newself, SV *newtable, HV *ptr_map, UV old_table);

/*H::R API*/
void HRA_store_sk(SV *hr, SV *ukey, SV *value, ...);
SV* HRA_fetch_sk(SV *hr, SV *ukey); //we manipulate perl's stack in this one

void HRA_store_a(SV *hr, SV *attr, char *t, SV *value, ...);
void  HRA_fetch_a(SV *hr, SV *attr, char *t);
void HRA_dissoc_a(SV *hr, SV *attr, char *t, SV *value);
void HRA_unlink_a(SV *hr, SV *attr, char *t);
SV* HRA_attr_get(SV *hr, SV *attr, char *t);
void HRA_ithread_store_lookup_info(SV *self, HV *ptr_map);

#endif /*HREG_H_*/