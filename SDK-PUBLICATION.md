# Changed-Block-Tracking-SDK — SDK et installation

Projet : [Changed-Block-Tracking-SDK](https://github.com/titanfyre/Changed-Block-Tracking-SDK). [Sources et documentation des SDK](https://github.com/TitanCSA/sdk-packages/tree/main/projects/Changed-Block-Tracking-SDK).

Choisis le paquet a installer. Installe son outil habituel (Node.js/npm, Python/pip, .NET, etc.), puis execute la commande dans le dossier de ton application. Les pilotes et applications requis restent a installer separement selon le README du projet.

Les SDK natifs sont destines a Windows x64. Java utilise Java22+ (Kotlin passe par le SDK Java). Flutter utilise le SDK Dart ; le chargement de la DLL doit etre configure dans l application. AutoHotInterceptor .NET utilise .NET Framework4.8.

| Paquet | Registre | Disponibilite | Installation / utilisation |
| --- | --- | --- | --- |
| `changed-block-tracking-sdk` | vcpkg | Registre TitanCSA disponible (configuration ci-dessous) | `vcpkg install changed-block-tracking-sdk:x64-windows` |
| `changed-block-tracking-sdk` | Conan | Registre TitanCSA disponible (configuration ci-dessous) | `conan install --requires=changed-block-tracking-sdk/0.1.1 -r titancsa --build=missing -s compiler.cppstd=17` |
| `Changed-Block-Tracking-SDK.Native` | NuGet | Publie et verifie | `Install-Package Changed-Block-Tracking-SDK.Native -Version 0.1.1` |
| `ChangedBlockTrackingSDK.SDK` | NuGet | Publie et verifie | `dotnet add package ChangedBlockTrackingSDK.SDK --version 1.0.0` |
| `changed_block_tracking_sdk` | PyPI | Pas encore disponible / validation en cours | Commande prevue apres publication : `python -m pip install changed-block-tracking-sdk==1.0.0` |
| `changed-block-tracking-sdk` | Conda | Canal TitanCSA disponible ; acceptation conda-forge encore en attente | `conda install --override-channels -c https://raw.githubusercontent.com/TitanCSA/sdk-packages/089bc73095aa8eda89bacbe10d0b9e58394b0207/conda-channel -c conda-forge changed-block-tracking-sdk` |
| `changed_block_tracking_sdk` | crates.io | Publie et verifie | `cargo add changed-block-tracking-sdk@1.0.0` |
| `changed_block_tracking_sdk` | npm | Pas encore disponible / validation en cours | Commande prevue apres publication : `npm install changed-block-tracking-sdk@1.0.0` |
| `io.github.titancsa:changed-block-tracking-sdk` | Maven Central | Publie et verifie | `io.github.titancsa:changed-block-tracking-sdk:1.0.0 (pom.xml / Gradle)` |
| `com.github.titanfyre:Changed-Block-Tracking-SDK` | JitPack | Publie et verifie | `com.github.titanfyre:Changed-Block-Tracking-SDK:java-sdk-v1.0.0 (avec le depot JitPack)` |
| `github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules` | Go Modules | Publie et verifie | `go get github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules@v1.0.0` |
| `github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules` | pkg.go.dev | Publie et verifie | `Documentation : https://pkg.go.dev/github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules@v1.0.0` |
| `changed_block_tracking_sdk` | PowerShell Gallery | Publie et verifie | `Install-Module ChangedBlockTrackingSDK -RequiredVersion 1.0.0 -Scope CurrentUser` |
| `changed_block_tracking_sdk` | pub.dev | Pas encore disponible / validation en cours | Commande prevue apres publication : `dart pub add changed_block_tracking_sdk:1.0.0` |
| `changed_block_tracking_sdk` | RubyGems | Publie et verifie | `gem install changed-block-tracking-sdk -v 1.0.0` |
| `changed_block_tracking_sdk` | Packagist | Pas encore disponible / validation en cours | Commande prevue apres publication : `composer require titancsa/changed-block-tracking-sdk:1.0.0` |

## Configuration vcpkg et Conan

Les sources et headers C / C++ sont dans le dossier central de ce projet. Les headers C se trouvent dans les dossiers `*-C-ffi` ou `*-C-json` (Interceptor-MultiLang : `native/`). Les paquets natifs C++ et la disponibilite du bridge C sont distincts ; consulte leur README avant de lier une application C.

Pour vcpkg, ajoute un fichier `vcpkg-configuration.json` a cote de ton manifeste `vcpkg.json` :

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/TitanCSA/vcpkg-registry",
      "baseline": "9a4b8cb920d4cfc97ce6d7af461bcc7508392a68",
      "packages": [
        "changed-block-tracking-sdk",
        "changed-block-tracking-sdk-sdk"
      ]
    }
  ]
}
```

Pour Conan2, configure une seule fois l index source :

```powershell
git clone https://github.com/TitanCSA/sdk-packages.git
conan remote add titancsa ./sdk-packages/conan-index -t local-recipes-index
```

Execute ensuite la commande Conan du tableau. Ce registre construit les bibliotheques depuis leurs sources ; il est distinct de ConanCenter. La commande NuGet native s execute dans la console du gestionnaire de packages de Visual Studio.

## Configuration Maven et JitPack

Dans le bloc `<dependencies>` de ton `pom.xml` :

```xml
<dependency>
  <groupId>io.github.titancsa</groupId>
  <artifactId>changed-block-tracking-sdk</artifactId>
  <version>1.0.0</version>
</dependency>
```

Avec Gradle Kotlin :

```kotlin
implementation("io.github.titancsa:changed-block-tracking-sdk:1.0.0")
```

Pour JitPack, ajoute aussi le depot `https://jitpack.io`, puis utilise les coordonnees du tableau. Les versions Go et JitPack deja publiees continuent de pointer vers leurs anciens tags immuables dans le depot du projet : ces tags restent disponibles. Leurs sources de travail sont maintenant centralisees ici ; aucun ancien import Go n est renomme.

## Etat et documentation

Les commandes marquees « prevues » ne sont pas encore des installations disponibles. Le canal Conda TitanCSA n est pas le catalogue officiel conda-forge. Les versions et licences existantes ne sont pas remplacees par cette reorganisation.

Les guides API et exemples de chaque langage se trouvent dans leurs dossiers SDK, a cote des sources. Les bindings peuvent exposer une partie de l API C++ ; les limites restent documentees dans chaque SDK.

[Retour au catalogue central](../../README.md)
