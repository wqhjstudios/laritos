#include <log.h>

#include <stdint.h>
#include <stdbool.h>
#include <test/test.h>
#include <fs/vfs/core.h>
#include <fs/vfs/types.h>
#include <fs/pseudofs.h>

T(pseudofs_fstype_is_supported_by_default) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_mount_populates_superblock) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));
    tassert(fsm->sb != NULL);
    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_dir_creation_creates_inodes_and_dentries) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1/dir2"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_remove_non_existent_dir) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    tassert(vfs_remove_dir(fsm->root, "hola") < 0);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1/dir2"));

    tassert(vfs_remove_dir(fsm->root, "dir1") >= 0);
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
    tassert(!vfs_dentry_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_dir_removal_deletes_inodes_and_dentries2) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1"));

    fs_dentry_t *dir2 = vfs_create_dir(dir1, "dir2", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir2 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1/dir2"));

    tassert(vfs_remove_dir(dir1, "dir2") >= 0);
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
    tassert(vfs_dentry_exist("/test/dir1"));

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
    tassert(!vfs_dentry_exist("/test/dir1/dir2"));
TEND

T(pseudofs_cannot_create_dir_if_file_or_dir_already_existing) {
    fs_mount_t *fsm = vfs_mount_fs("pseudofs", "/test", FS_MOUNT_READ | FS_MOUNT_WRITE, NULL);
    tassert(fsm != NULL);
    tassert(vfs_dentry_exist("/test"));

    fs_dentry_t *dir1 = vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE);
    tassert(dir1 != NULL);
    tassert(vfs_dentry_is_dir("/test/dir1"));

    tassert(vfs_create_dir(fsm->root, "dir1", FS_ACCESS_MODE_READ | FS_ACCESS_MODE_WRITE) == NULL);

    tassert(vfs_unmount_fs("/test") >= 0);
    tassert(!vfs_dentry_exist("/test"));
    tassert(!vfs_dentry_exist("/test/dir1"));
TEND
