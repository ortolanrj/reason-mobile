open Helper;

[@deriving yojson]
type manifest = {
  source: [@default None] option(list(list(string))),
  build: [@default None] option(list(list(string))),
  install: [@default None] option(list(list(string))),
  build_env: [@default None] option(StringMap.t(option(string))),
  exported_env: [@default None] option(StringMap.t(option(string))),
  dependencies: [@default None] option(StringMap.t(string)),
  raw_dependencies: [@default None] option(StringMap.t(string)),
};

type t = {
  manifest,
  checksum: option(string),
  files_folder: option(string),
};

let patches_folder = {
  let default =
    Filename.concat(
      Filename.current_dir_name,
      Filename.concat("..", "patches"),
    );
  let.apply () = Option.value(~default);
  Sys.getenv_opt("ESY__GENERATE_PATCHES");
};

// TODO: think a little bit better if using .none as early return is the best idea
let get_patch_folder = (name, version) => {
  let name_and_version = name ++ "." ++ version;
  let folders = Sys.readdir(patches_folder) |> Array.to_list;
  let.some name = {
    // TODO: read the version from the original manifest
    let.none () = folders |> List.find_opt((==)(name_and_version));
    folders |> List.find_opt((==)(name));
  };
  Filename.concat(patches_folder, name) |> some;
};

let get_path = (name, version) => {
  switch (get_patch_folder(name, version)) {
  | Some(folder) =>
    let files_folder = Filename.concat(folder, "files");
    let.await files_folder_exists = Lwt_unix.file_exists(files_folder);
    let.await checksum = {
      files_folder_exists
        ? Lib.folder_sha1(files_folder) |> Lwt.map(some) : await(None);
    };

    let.await manifest = {
      let.await data =
        Filename.concat(folder, "package.json") |> Lib.read_file;
      Yojson.Safe.from_string(data)
      |> manifest_of_yojson
      |> Result.get_ok
      |> await;
    };

    {
      manifest,
      checksum,
      files_folder: files_folder_exists ? Some(files_folder) : None,
    }
    |> some
    |> await;
  | None => await(none)
  };
};
