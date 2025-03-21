import com.android.build.api.dsl.Packaging

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.krisp3t.retrorenderer"
    compileSdk = 35
    ndkVersion = "19.2.5345600"
    defaultConfig {
        applicationId = "com.krisp3t.retrorenderer"
        minSdk = 29
        targetSdk = 35
        versionCode = 1
        versionName = "0.0.1"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                // TODO: use vcpkg set in cmake
                val vcpkgRoot = requireNotNull(System.getenv("VCPKG_ROOT")) {
                    "VCPKG_ROOT environment variable is not set. Install vcpkg or set the variable."
                }
                val vcpkgToolchain = "$vcpkgRoot/scripts/buildsystems/vcpkg.cmake"

                arguments += listOf(
                    "--preset=debug-arm64-android",
                    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
                )
                abiFilters += listOf("arm64-v8a")
            }
        }

    }
    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            buildStagingDirectory = file("../../build-android")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        debug {
            isDebuggable = true
            isJniDebuggable = true
            isMinifyEnabled = false
            isShrinkResources = false
            packaging {
                jniLibs.keepDebugSymbols += "**/*.so"
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        prefab = true
    }
    lint {
        abortOnError = false
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}