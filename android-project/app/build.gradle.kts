import com.android.build.api.dsl.Packaging

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.krisp3t.retrorenderer"
    compileSdk = 35
    defaultConfig {
        applicationId = "com.krisp3t.retrorenderer"
        minSdk = 27
        targetSdk = 35
        versionCode = 1
        versionName = "0.0.1"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                abiFilters += listOf("arm64-v8a")
                arguments += listOf(
                    "-DCMAKE_TOOLCHAIN_FILE=${projectDir}/../../extern/vcpkg/scripts/buildsystems/vcpkg.cmake"
                )
            }
        }

    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            externalNativeBuild {
                cmake {
                    arguments += listOf("--preset=release-arm64-android")
                }
            }
        }
        debug {
            isDebuggable = true
            isJniDebuggable = true
            isMinifyEnabled = false
            isShrinkResources = false
            packaging {
                jniLibs.keepDebugSymbols += "**/*.so"
            }
            externalNativeBuild {
                cmake {
                    arguments += listOf("--preset=debug-arm64-android")
                }
            }
        }
    }
    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            buildStagingDirectory = file("../../out/android-cmake")
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