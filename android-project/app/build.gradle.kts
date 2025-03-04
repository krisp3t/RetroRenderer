import com.android.build.api.dsl.Packaging

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.krisp3t.retrorenderer"
    compileSdk = 35
    ndkVersion = "28.0.13004108"
    defaultConfig {
        applicationId = "com.krisp3t.retrorenderer"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "0.0.1"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_APP_PLATFORM=android-21", 
                    "-DANDROID_STL=c++_shared",
                    "-DVCPKG_TRACE_FIND_PACKAGE=ON",
                    "-DCMAKE_PREFIX_PATH=D:/Programming/RetroRenderer/build-android/vcpkg_installed/arm64-android",
                    "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/Users/krisp3t/AppData/Local/Android/Sdk/ndk/28.0.13004108/build/cmake/android.toolchain.cmake",
                    "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake",
                    "-DVCPKG_CMAKE_SYSTEM_NAME=Android",
                    "-DVCPKG_TARGET_TRIPLET=arm64-android",
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