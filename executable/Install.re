open Fnm;

let mkDownloadsDir = () => {
  let exists = Lwt_unix.file_exists(Directories.downloads);
  if%lwt (exists |> Lwt.map(x => !x)) {
    Logger.debug(
      <Pastel>
        "Creating "
        <Pastel color=Pastel.Cyan> Directories.downloads </Pastel>
        " for the first time"
      </Pastel>,
    );
    let%lwt _ = System.mkdirp(Directories.downloads);
    Lwt.return();
  } else {
    Lwt.return();
  };
};

let main = (~version as versionName) => {
  let%lwt os = System.NodeOS.get()
  and arch = System.NodeArch.get()
  and versionName =
    switch (versionName) {
    | Some(versionName) => Lwt.return(versionName)
    | None => Dotfiles.getVersion()
    };

  let versionName = Versions.format(versionName);
  let%lwt _ = Versions.throwIfInstalled(versionName);

  Logger.debug(
    <Pastel>
      "Looking for node "
      <Pastel color=Pastel.Cyan> versionName </Pastel>
      " for "
      <Pastel color=Pastel.Cyan>
        {System.NodeOS.toString(os)}
        " "
        {System.NodeArch.toString(arch)}
      </Pastel>
    </Pastel>,
  );

  let%lwt (versionName, filepath) =
    Versions.getFileToDownload(~version=versionName, ~os, ~arch);

  let%lwt _ = Versions.throwIfInstalled(versionName);

  let tarDestination =
    Filename.concat(
      Directories.downloads,
      versionName ++ Versions.Remote.downloadFileSuffix,
    );

  Logger.debug(
    <Pastel>
      "Downloading "
      <Pastel color=Pastel.Cyan> filepath </Pastel>
      " to "
      <Pastel color=Pastel.Cyan> tarDestination </Pastel>
    </Pastel>,
  );

  let%lwt _ = System.mkdirp(Filename.dirname(tarDestination));
  let%lwt _ = Http.download(filepath, ~into=tarDestination);
  let extractionDestination =
    Filename.concat(Directories.nodeVersions, versionName);

  Logger.debug(
    <Pastel>
      "Extracting "
      <Pastel color=Pastel.Cyan> tarDestination </Pastel>
      " to "
      <Pastel color=Pastel.Cyan> extractionDestination </Pastel>
    </Pastel>,
  );

  Logger.info(
    <Pastel>
      "Version "
      <Pastel color=Pastel.Cyan> versionName </Pastel>
      " was successfuly downloaded"
    </Pastel>,
  );

  let%lwt _ =
    Compression.extractFile(tarDestination, ~into=extractionDestination);

  Lwt.return();
};

let run = (~version) =>
  try%lwt (main(~version)) {
  | Versions.No_Download_For_System(os, arch) =>
    Logger.error(
      <Pastel>
        "Version exists, but can't find a file for your system:\n"
        "  OS:           "
        <Pastel color=Pastel.Cyan> {System.NodeOS.toString(os)} </Pastel>
        "\n"
        "  Architecture: "
        <Pastel color=Pastel.Cyan> {System.NodeArch.toString(arch)} </Pastel>
      </Pastel>,
    );
    exit(1);
  | Versions.Already_installed(version) =>
    Logger.error(
      <Pastel>
        "Version "
        <Pastel color=Pastel.Cyan> version </Pastel>
        " is already installed."
      </Pastel>,
    )
    |> Lwt.return
  | Versions.Version_not_found(version) =>
    Logger.error(
      <Pastel>
        "Version "
        <Pastel color=Pastel.Cyan> version </Pastel>
        " not found!"
      </Pastel>,
    );
    exit(1);
  };
