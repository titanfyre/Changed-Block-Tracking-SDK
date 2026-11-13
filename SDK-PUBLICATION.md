# SDK publication status

Current source layout and registry coverage. A GitHub download is distinct from publication on a central registry.

| Target | Status | Package / documentation |
| --- | --- | --- |
| CPP/vcpkg | published_custom_registry | changed-block-tracking-sdk |
| CPP/conan | published_custom_source_index | [changed-block-tracking-sdk](https://github.com/TitanCSA/sdk-packages/tree/089bc73095aa8eda89bacbe10d0b9e58394b0207/conan-index) [Source](packaging/native/) |
| CPP/nuget | account_email_confirmed_requires_matching_trusted_publishing_policy | Changed-Block-Tracking-SDK.Native |
| NET/nuget | built_verified_requires_trusted_publishing_policy | ChangedBlockTrackingSDK.SDK [Source](Changed-Block-Tracking-SDK-NET-nuget/) |
| Python/pypi | built_fixture_and_production_verified | changed-block-tracking-sdk [Source](Changed-Block-Tracking-SDK-Python-pypi/) |
| Python/conda-forge | custom_channel_install_verified_requires_conda_forge_review | [changed-block-tracking-sdk](https://raw.githubusercontent.com/TitanCSA/sdk-packages/089bc73095aa8eda89bacbe10d0b9e58394b0207/conda-channel) [Source](packaging/conda/) |
| Rust/cargo | published_install_verified | [changed-block-tracking-sdk](https://crates.io/crates/changed-block-tracking-sdk) [Source](Changed-Block-Tracking-SDK-Rust-cargo/) |
| JavaScript/npm | awaiting_browser_publication_authentication | [changed-block-tracking-sdk](https://www.npmjs.com/package/changed-block-tracking-sdk) [Source](Changed-Block-Tracking-SDK-JavaScript-npm/) |
| Java/maven | published_install_verified | [changed-block-tracking-sdk](https://central.sonatype.com/artifact/io.github.titancsa/changed-block-tracking-sdk/1.0.0) [Source](Changed-Block-Tracking-SDK-Java-maven/) |
| Java/jitpack | published_install_verified | [changed-block-tracking-sdk](https://jitpack.io/#titanfyre/Changed-Block-Tracking-SDK/java-sdk-v1.0.0) [Source](Changed-Block-Tracking-SDK-Java-maven/) |
| Go/modules | published_install_verified | [changed-block-tracking-sdk](https://pkg.go.dev/github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules@v1.0.0) [Source](Changed-Block-Tracking-SDK-Go-modules/) |
| Go/pkg-go-dev | indexed_verified | [Documentation](https://pkg.go.dev/github.com/titanfyre/Changed-Block-Tracking-SDK/Changed-Block-Tracking-SDK-Go-modules@v1.0.0) |
| PowerShell/gallery | published_install_verified | [ChangedBlockTrackingSDK](https://www.powershellgallery.com/packages/ChangedBlockTrackingSDK/1.0.0) [Source](Changed-Block-Tracking-SDK-PowerShell-gallery/) |
| Dart/pub-dev | built_fixture_and_production_verified | changed_block_tracking_sdk [Source](Changed-Block-Tracking-SDK-Dart-pub-dev/) |
| Ruby/rubygems | published_install_verified | [changed-block-tracking-sdk](https://rubygems.org/gems/changed-block-tracking-sdk) [Source](Changed-Block-Tracking-SDK-Ruby-rubygems/) |
| PHP/packagist | composer_source_and_tag_published | [titancsa/changed-block-tracking-sdk](https://packagist.org/packages/titancsa/changed-block-tracking-sdk) |

Native SDK source and CMake/Conan/vcpkg/NuGet recipes share [`packaging/native`](packaging/native/LAYOUT.md). Previous release archives and tags remain unchanged.

Application and driver prerequisites and original licensing remain those documented by each SDK. No driver installer is bundled with these source packages.
