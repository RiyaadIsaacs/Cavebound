import unreal

FBX_PATH = r"C:\Users\khosa\Videos\Shows\Fun Stuff\KayKit_Skeletons_1.1_FREE\KayKit_Skeletons_1.1_FREE\characters\fbx\Skeleton_Rogue.fbx"
DEST_PATH = "/Game/Characters/Skeleton"
ASSET_NAME = "Skeleton_Rogue"
SKELETON_PATH = "/Game/Characters/Skeleton/Skeleton_Minion_Skeleton.Skeleton_Minion_Skeleton"


def main():
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        unreal.log_error(f"Missing skeleton: {SKELETON_PATH}")
        return

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", FBX_PATH)
    task.set_editor_property("destination_path", DEST_PATH)
    task.set_editor_property("destination_name", ASSET_NAME)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("skeleton", skeleton)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("create_physics_asset", False)

    skel_data = options.get_editor_property("skeletal_mesh_import_data")
    if skel_data:
        skel_data.set_editor_property("import_morph_targets", False)
        skel_data.set_editor_property("use_t0_as_ref_pose", True)
        skel_data.set_editor_property("import_meshes_in_bone_hierarchy", True)
        skel_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)

    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh_path = f"{DEST_PATH}/{ASSET_NAME}.{ASSET_NAME}"
    mesh = unreal.load_asset(mesh_path)
    if not mesh:
        unreal.log_error(f"Import failed — asset missing at {mesh_path}")
        for err in task.get_editor_property("result"):
            unreal.log_error(str(err))
        return

    # Ensure shared KayKit skeleton so ABP_Minion / montages retarget cleanly.
    if hasattr(mesh, "set_skeleton"):
        mesh.set_skeleton(skeleton)
    elif hasattr(mesh, "skeleton"):
        mesh.set_editor_property("skeleton", skeleton)

    texture = unreal.load_asset("/Game/Characters/Skeleton/skeleton_texture.skeleton_texture")
    if texture:
        for idx in range(mesh.get_num_sections(0) if hasattr(mesh, "get_num_sections") else 1):
            pass
        mats = mesh.get_editor_property("materials") if mesh else None
        unreal.log(f"Rogue imported: {mesh.get_path_name()} skeleton={skeleton.get_name()}")

    unreal.EditorAssetLibrary.save_asset(mesh_path, only_if_is_dirty=False)
    unreal.log("Skeleton_Rogue import complete")


if __name__ == "__main__":
    main()
