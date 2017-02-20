// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
  void *va = ROUNDDOWN(addr, PGSIZE);
  const pte_t *pte = (const pte_t *) uvpt + PGNUM(va);
	uint32_t err = utf->utf_err;
	int rc, perm;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
  // TODO: remove this later for disk write
  if (*pte & PTE_W)
    panic("pgfault at PTE_W!");

  if (!(*pte & PTE_COW))
    panic("pgfault at non PTE_COW!");

  perm = PTE_U | PTE_W | PTE_P;

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
  rc = sys_page_alloc(0, UTEMP, perm);
  if (rc < 0)
    panic("Unable to alloc at UTEMP!");

  memcpy(UTEMP, va, PGSIZE);
  rc = sys_page_map(0, UTEMP, 0, va, perm);
  if (rc < 0)
    panic("Unable to map from UTEMP to va!");

  rc = sys_page_unmap(0, UTEMP);
  if (rc < 0)
    panic("Unable to unmap UTEMP!");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t eid, size_t vpn)
{
	int rc;

	// LAB 4: Your code here.
  void *va = (void *) (vpn << PGSHIFT);
  const pte_t *pte = (const pte_t *) uvpt + vpn;

  if ((*pte & PTE_W) || (*pte & PTE_COW)) {

    // map child first, then parent.
    int perm = PTE_COW | PTE_U | PTE_P;
    rc = sys_page_map(0, va, eid, va, perm);
    if (rc < 0)
      return -1;

    rc = sys_page_map(0, va, 0, va, perm);
    if (rc < 0)
      return -1;

  } else {

    int perm = PTE_U | PTE_P;
    rc = sys_page_map(0, va, eid, va, perm);
    if (rc < 0)
      return -1;
  }

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
  set_pgfault_handler(pgfault);

  int eid = sys_exofork();
  if (eid < 0)
    panic("sys_exofork failure!");

  /* child */
  if (eid == 0) {
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  /* parent */
  int rc;
  size_t pdx, ptx, vpn;
  // this makes it easy to check page tables
  static_assert(UTOP % PTSIZE == 0);
  size_t pdx_ub = PDX(UTOP);

  // iterate over pdx, ptx to find all page tables present.
  for (pdx = 0; pdx < pdx_ub; pdx++) {
    const pde_t *pde = (const pde_t *) uvpd + pdx;
    if (!(*pde & PTE_P))
      continue;

    for (ptx = 0; ptx < NPTENTRIES; ptx++) {
      vpn = (pdx << PTXWIDTH) | ptx;
      const pte_t *pte = (const pte_t *) uvpt + vpn;
      if (!(*pte & PTE_P))
        continue;
      if (vpn << PGSHIFT == UXSTACKTOP - PGSIZE)
        continue;

      // duppage page table for child env.
      // change PTE_W to PTE_COW for itself.
      duppage(eid, vpn);
    }
  }

  // alloc uxstack for child
  rc = sys_page_alloc(eid, (void*) UXSTACKTOP-PGSIZE,
                      PTE_U | PTE_W | PTE_P);
  if (rc < 0)
    goto forkbad;

  // setup pgfault_upcall for child
  if (sys_env_set_pgfault_upcall(eid, thisenv->env_pgfault_upcall) < 0)
    goto forkbad;

  // make child runnable
  if (sys_env_set_status(eid, ENV_RUNNABLE) < 0)
    goto forkbad;

  return eid;

forkbad:
  sys_env_destroy(eid);
  return -1;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
