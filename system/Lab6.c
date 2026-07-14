/* Lab6.c - experiment 6 simple disk FS and external ELF loader */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

struct k2023202316_ext_args {
  uint32 argc;
  char data[K2023202316_EXT_ARG_BYTES];
  uint32 off[K2023202316_EXT_MAX_ARGS];
};

local uint32 k2023202316_load32(uint8 *);
local status k2023202316_fs_read_bytes(uint32, void *, uint32);
local status k2023202316_fs_write_bytes(uint32, void *, uint32);
local status k2023202316_fs_format(uint32 *, uint32 *);
local status k2023202316_fs_get_dir(uint32, uint32,
                                    struct k2023202316_fs_dirent *);
local status k2023202316_copy_user_string(uint32, char *, uint32);
local status k2023202316_copy_user_args(uint32, uint32,
                                        struct k2023202316_ext_args *);
local syscall k2023202316_exec_external(struct k2023202316_trapframe *, char *,
                                        struct k2023202316_ext_args *);
local status k2023202316_read_elf_image(char *, char **, uint32 *);
local status k2023202316_validate_elf_image(char *, uint32, uint32 *);
local syscall k2023202316_map_elf_image(pid32, char *, uint32, uint32 *);
local uint32 k2023202316_build_arg_stack(pid32, struct k2023202316_ext_args *);
local uint32 k2023202316_strlen(char *);
local void k2023202316_set_proc_name(pid32, char *);

/*------------------------------------------------------------------------
 * k2023202316_fs_list - print all files in the fs_util image
 *------------------------------------------------------------------------
 */
status k2023202316_fs_list(void) {
  uint32 nfiles;
  uint32 entsize;
  uint32 i;
  struct k2023202316_fs_dirent ent;

  if (k2023202316_fs_format(&nfiles, &entsize) == SYSERR) {
    printf("Lab6: cannot read disk directory\n");
    return SYSERR;
  }
  printf("name                             size       offset\n");
  for (i = 0; i < nfiles; i++) {
    if (k2023202316_fs_get_dir(i, entsize, &ent) == SYSERR) {
      return SYSERR;
    }
    printf("%-32s %10u 0x%08X\n", ent.name, ent.size, ent.offset);
  }
  return OK;
}

/*------------------------------------------------------------------------
 * k2023202316_fs_find - find a file by name in the fs_util image
 *------------------------------------------------------------------------
 */
status k2023202316_fs_find(char *name, struct k2023202316_fs_dirent *out) {
  uint32 nfiles;
  uint32 entsize;
  uint32 i;
  char *target;
  struct k2023202316_fs_dirent ent;

  if (name == NULL || out == NULL) {
    return SYSERR;
  }
  target = name;
  if (target[0] == '/') {
    target++;
  }
  if (k2023202316_fs_format(&nfiles, &entsize) == SYSERR) {
    return SYSERR;
  }
  for (i = 0; i < nfiles; i++) {
    if (k2023202316_fs_get_dir(i, entsize, &ent) == SYSERR) {
      return SYSERR;
    }
    if (strncmp(ent.name, target, K2023202316_FS_NAME_LEN) == 0) {
      *out = ent;
      return OK;
    }
  }
  return SYSERR;
}

/*------------------------------------------------------------------------
 * k2023202316_fs_read - read bytes from a named file
 *------------------------------------------------------------------------
 */
int32 k2023202316_fs_read(char *name, uint32 off, void *buf, uint32 count) {
  struct k2023202316_fs_dirent ent;

  if (buf == NULL || k2023202316_fs_find(name, &ent) == SYSERR ||
      off > ent.size) {
    return SYSERR;
  }
  if (count > ent.size - off) {
    count = ent.size - off;
  }
  if (k2023202316_fs_read_bytes(ent.offset + off, buf, count) == SYSERR) {
    return SYSERR;
  }
  return count;
}

/*------------------------------------------------------------------------
 * k2023202316_fs_write - overwrite bytes in a named file
 *------------------------------------------------------------------------
 */
int32 k2023202316_fs_write(char *name, uint32 off, void *buf, uint32 count) {
  struct k2023202316_fs_dirent ent;

  if (buf == NULL || k2023202316_fs_find(name, &ent) == SYSERR ||
      off > ent.size) {
    return SYSERR;
  }
  if (count > ent.size - off) {
    count = ent.size - off;
  }
  if (k2023202316_fs_write_bytes(ent.offset + off, buf, count) == SYSERR) {
    return SYSERR;
  }
  return count;
}

/*------------------------------------------------------------------------
 * k2023202316_user_write_file - syscall helper for external program B
 *------------------------------------------------------------------------
 */
syscall k2023202316_user_write_file(char *uname, char *ubuf, uint32 count) {
  char name[K2023202316_FS_NAME_LEN];
  char data[K2023202316_HD_SECSIZE];

  if (count > sizeof(data)) {
    count = sizeof(data);
  }
  if (k2023202316_copy_user_string((uint32)uname, name, sizeof(name)) ==
          SYSERR ||
      k2023202316_copy_from_user(currpid, data, (uint32)ubuf, count) ==
          SYSERR) {
    return SYSERR;
  }
  return k2023202316_fs_write(name, 0, data, count);
}

/*------------------------------------------------------------------------
 * k2023202316_execfile_from_user - syscall helper for external exec
 *------------------------------------------------------------------------
 */
syscall k2023202316_execfile_from_user(struct k2023202316_trapframe *tf,
                                       char *uname, uint32 argc,
                                       char **uargv) {
  char name[K2023202316_FS_NAME_LEN];
  struct k2023202316_ext_args args;

  if (argc == 0 || argc > K2023202316_EXT_MAX_ARGS ||
      k2023202316_copy_user_string((uint32)uname, name, sizeof(name)) ==
          SYSERR ||
      k2023202316_copy_user_args(argc, (uint32)uargv, &args) == SYSERR) {
    return SYSERR;
  }
  return k2023202316_exec_external(tf, name, &args);
}

local status k2023202316_fs_read_bytes(uint32 off, void *buf, uint32 count) {
  uint8 sector[K2023202316_HD_SECSIZE];
  uint8 *dst;
  uint32 done;
  uint32 lba;
  uint32 soff;
  uint32 chunk;

  if (off > K2023202316_FS_IMAGE_SIZE ||
      count > K2023202316_FS_IMAGE_SIZE - off) {
    return SYSERR;
  }
  dst = (uint8 *)buf;
  for (done = 0; done < count; done += chunk) {
    lba = (off + done) / K2023202316_HD_SECSIZE;
    soff = (off + done) % K2023202316_HD_SECSIZE;
    chunk = K2023202316_HD_SECSIZE - soff;
    if (chunk > count - done) {
      chunk = count - done;
    }
    if (read(HD0, (char *)sector, lba) == SYSERR) {
      return SYSERR;
    }
    memcpy(dst + done, sector + soff, chunk);
  }
  return OK;
}

local status k2023202316_fs_write_bytes(uint32 off, void *buf, uint32 count) {
  uint8 sector[K2023202316_HD_SECSIZE];
  uint8 *src;
  uint32 done;
  uint32 lba;
  uint32 soff;
  uint32 chunk;

  if (off > K2023202316_FS_IMAGE_SIZE ||
      count > K2023202316_FS_IMAGE_SIZE - off) {
    return SYSERR;
  }
  src = (uint8 *)buf;
  for (done = 0; done < count; done += chunk) {
    lba = (off + done) / K2023202316_HD_SECSIZE;
    soff = (off + done) % K2023202316_HD_SECSIZE;
    chunk = K2023202316_HD_SECSIZE - soff;
    if (chunk > count - done) {
      chunk = count - done;
    }
    if (read(HD0, (char *)sector, lba) == SYSERR) {
      return SYSERR;
    }
    memcpy(sector + soff, src + done, chunk);
    if (write(HD0, (char *)sector, lba) == SYSERR) {
      return SYSERR;
    }
  }
  return OK;
}

local status k2023202316_fs_format(uint32 *nfiles, uint32 *entsize) {
  uint8 buf[K2023202316_HD_SECSIZE];
  uint32 n32;
  uint32 n64;

  if (read(HD0, (char *)buf, 0) == SYSERR) {
    return SYSERR;
  }
  n32 = k2023202316_load32(buf + 4);
  n64 = k2023202316_load32(buf + 8);
  if (n32 > 0 && n32 <= K2023202316_FS_MAX_FILES) {
    *nfiles = n32;
    *entsize = 136;
    return OK;
  }
  if (n64 > 0 && n64 <= K2023202316_FS_MAX_FILES) {
    *nfiles = n64;
    *entsize = 144;
    return OK;
  }
  return SYSERR;
}

local status k2023202316_fs_get_dir(uint32 index, uint32 entsize,
                                    struct k2023202316_fs_dirent *ent) {
  uint8 raw[144];
  uint32 base;

  base = (index + 1) * entsize;
  if (k2023202316_fs_read_bytes(base, raw, entsize) == SYSERR) {
    return SYSERR;
  }
  if (entsize == 136) {
    ent->offset = k2023202316_load32(raw);
    ent->size = k2023202316_load32(raw + 4);
    memcpy(ent->name, raw + 8, K2023202316_FS_NAME_LEN);
  } else {
    ent->offset = k2023202316_load32(raw);
    ent->size = k2023202316_load32(raw + 8);
    memcpy(ent->name, raw + 16, K2023202316_FS_NAME_LEN);
  }
  ent->name[K2023202316_FS_NAME_LEN - 1] = NULLCH;
  if (ent->offset > K2023202316_FS_IMAGE_SIZE ||
      ent->size > K2023202316_FS_IMAGE_SIZE - ent->offset) {
    return SYSERR;
  }
  return OK;
}

local syscall k2023202316_exec_external(struct k2023202316_trapframe *tf,
                                        char *name,
                                        struct k2023202316_ext_args *args) {
  uint32 entry;
  uint32 newsp;
  char *image;
  uint32 image_size;

  if (k2023202316_read_elf_image(name, &image, &image_size) == SYSERR) {
    return SYSERR;
  }
  if (k2023202316_validate_elf_image(image, image_size, &entry) == SYSERR) {
    freemem(image, image_size);
    return SYSERR;
  }

  if (k2023202316_reset_user_space(currpid) == SYSERR ||
      k2023202316_map_elf_image(currpid, image, image_size, &entry) ==
          SYSERR) {
    freemem(image, image_size);
    kill(currpid);
    return SYSERR;
  }
  freemem(image, image_size);

  newsp = k2023202316_build_arg_stack(currpid, args);
  if (newsp == 0) {
    kill(currpid);
    return SYSERR;
  }
  k2023202316_set_proc_name(currpid, name);
  k2023202316_switch_addrspace(currpid);
  tf->eip = entry;
  tf->useresp = newsp;
  tf->eax = 0;
  return OK;
}

local status k2023202316_read_elf_image(char *name, char **image_out,
                                        uint32 *size_out) {
  struct k2023202316_fs_dirent ent;
  char *image;

  if (k2023202316_fs_find(name, &ent) == SYSERR ||
      ent.size < sizeof(struct elfhdr)) {
    return SYSERR;
  }
  image = getmem(ent.size);
  if (image == (char *)SYSERR) {
    return SYSERR;
  }
  if (k2023202316_fs_read(name, 0, image, ent.size) == SYSERR) {
    freemem(image, ent.size);
    return SYSERR;
  }
  *image_out = image;
  *size_out = ent.size;
  return OK;
}

local status k2023202316_validate_elf_image(char *image, uint32 image_size,
                                            uint32 *entry) {
  struct elfhdr *elf;
  struct proghdr *ph;
  bool8 entry_loaded;
  uint32 i;

  if (image == NULL || image_size < sizeof(struct elfhdr)) {
    return SYSERR;
  }
  elf = (struct elfhdr *)image;
  if (elf->magic != ELF_MAGIC || elf->phentsize != sizeof(struct proghdr) ||
      elf->phoff > image_size ||
      elf->phnum > (image_size - elf->phoff) / elf->phentsize ||
      elf->entry < K2023202316_ELF_BASE ||
      elf->entry >= K2023202316_ELF_LIMIT) {
    return SYSERR;
  }
  entry_loaded = FALSE;
  for (i = 0; i < elf->phnum; i++) {
    ph = (struct proghdr *)(image + elf->phoff + i * elf->phentsize);
    if (ph->type != ELF_PROG_LOAD) {
      continue;
    }
    if (ph->memsz == 0) {
      if (ph->filesz != 0) {
        return SYSERR;
      }
      continue;
    }
    if (ph->memsz < ph->filesz || ph->off > image_size ||
        ph->filesz > image_size - ph->off ||
        ph->vaddr < K2023202316_ELF_BASE ||
        ph->memsz > K2023202316_ELF_LIMIT - ph->vaddr) {
      return SYSERR;
    }
    if (elf->entry >= ph->vaddr && elf->entry < ph->vaddr + ph->memsz) {
      entry_loaded = TRUE;
    }
  }
  if (!entry_loaded) {
    return SYSERR;
  }
  *entry = elf->entry;
  return OK;
}

local syscall k2023202316_map_elf_image(pid32 pid, char *image,
                                        uint32 image_size, uint32 *entry) {
  struct elfhdr *elf;
  struct proghdr *ph;
  uint32 i;
  uint32 start;
  uint32 end;

  if (k2023202316_validate_elf_image(image, image_size, entry) == SYSERR) {
    return SYSERR;
  }
  elf = (struct elfhdr *)image;
  for (i = 0; i < elf->phnum; i++) {
    ph = (struct proghdr *)(image + elf->phoff + i * elf->phentsize);
    if (ph->type != ELF_PROG_LOAD) {
      continue;
    }
    if (ph->memsz == 0) {
      continue;
    }
    start = ph->vaddr & K2023202316_PAGE_MASK;
    end = (ph->vaddr + ph->memsz + K2023202316_PAGE_OFFSET) &
          K2023202316_PAGE_MASK;
    if (k2023202316_map_user_region(pid, start, end - start, "elf") ==
            SYSERR ||
        k2023202316_copy_to_user(pid, ph->vaddr, image + ph->off,
                                 ph->filesz) == SYSERR) {
      return SYSERR;
    }
  }
  return OK;
}

local uint32 k2023202316_build_arg_stack(pid32 pid,
                                         struct k2023202316_ext_args *args) {
  uint32 sp;
  uint32 value;
  uint32 i;
  uint32 len;
  uint32 argv[K2023202316_EXT_MAX_ARGS + 1];
  char *arg;
  uint32 argv_base;

  sp = K2023202316_USTACK_TOP;
  value = STACKMAGIC;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(value)) == SYSERR) {
    return 0;
  }

  for (i = args->argc; i > 0; i--) {
    arg = args->data + args->off[i - 1];
    len = k2023202316_strlen(arg) + 1;
    sp = (sp - len) & ~0x3;
    if (k2023202316_copy_to_user(pid, sp, arg, len) == SYSERR) {
      return 0;
    }
    argv[i - 1] = sp;
  }
  argv[args->argc] = 0;

  sp -= (args->argc + 1) * sizeof(uint32);
  argv_base = sp;
  if (k2023202316_copy_to_user(pid, argv_base, argv,
                               (args->argc + 1) * sizeof(uint32)) ==
      SYSERR) {
    return 0;
  }

  value = argv_base;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(value)) == SYSERR) {
    return 0;
  }
  value = args->argc;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(value)) == SYSERR) {
    return 0;
  }
  value = (uint32)u2023202316_exit;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(value)) == SYSERR) {
    return 0;
  }
  proctab[pid].pr2023202316_ustkptr = (char *)sp;
  return sp;
}

local status k2023202316_copy_user_args(
    uint32 argc, uint32 uargv, struct k2023202316_ext_args *args) {
  uint32 ptrs[K2023202316_EXT_MAX_ARGS];
  uint32 i;
  uint32 used;
  uint32 len;

  if (argc > K2023202316_EXT_MAX_ARGS ||
      k2023202316_copy_from_user(currpid, ptrs, uargv,
                                 argc * sizeof(uint32)) == SYSERR) {
    return SYSERR;
  }
  args->argc = argc;
  used = 0;
  for (i = 0; i < argc; i++) {
    args->off[i] = used;
    if (k2023202316_copy_user_string(ptrs[i], args->data + used,
                                     K2023202316_EXT_ARG_BYTES - used) ==
        SYSERR) {
      return SYSERR;
    }
    len = k2023202316_strlen(args->data + used) + 1;
    used += len;
    if (used >= K2023202316_EXT_ARG_BYTES && i + 1 < argc) {
      return SYSERR;
    }
  }
  return OK;
}

local status k2023202316_copy_user_string(uint32 uaddr, char *dst,
                                          uint32 maxlen) {
  uint32 i;
  char ch;

  if (uaddr == 0 || maxlen == 0) {
    return SYSERR;
  }
  for (i = 0; i < maxlen; i++) {
    if (k2023202316_copy_from_user(currpid, &ch, uaddr + i, 1) == SYSERR) {
      return SYSERR;
    }
    dst[i] = ch;
    if (ch == NULLCH) {
      return OK;
    }
  }
  dst[maxlen - 1] = NULLCH;
  return SYSERR;
}

local uint32 k2023202316_load32(uint8 *p) {
  return ((uint32)p[0]) | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) |
         ((uint32)p[3] << 24);
}

local uint32 k2023202316_strlen(char *s) {
  uint32 n;

  for (n = 0; s[n] != NULLCH; n++) {
    ;
  }
  return n;
}

local void k2023202316_set_proc_name(pid32 pid, char *name) {
  uint32 i;

  for (i = 0; i < PNMLEN - 1 && name[i] != NULLCH; i++) {
    proctab[pid].prname[i] = name[i];
  }
  proctab[pid].prname[i] = NULLCH;
}
