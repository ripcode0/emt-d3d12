# EMT Dependency Installer

### EMT Dependency Installer 
* `CMake` 기반으로 제작된 `CLI` 방식의 `C/C++` 라이브러리 설치 도구입니다.  
* 간단한 명령어와 설정만으로 외부 라이브러리를 자동 다운로드, 빌드, 설치할 수 있습니다.

### 특징

- `CMake` 기반으로 별도 스크립트 언어나 의존성 없음
- **CLI** 플래그를 통한 간편한 옵션 제어
- `Git` 저장소로부터 외부 라이브러리 자동 설치
- 프로젝트 통합을 위한 `prefix` 경로 지정 가능

### 요구사항

- **CMake 3.22** 이상
- **Git**
- **C++20** 또는 그 이상을 지원하는 컴파일러
- **Ninja** 또는 기타 **CMake** 호환 빌드 시스템

### 기본 사용법

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=external
cmake --build build --target install
```

### CLI 플래그

*  `--git     <url>`      설치할 라이브러리의 Git 주소  
    * `--git https://github.com/glfw/glfw.git`

* `--prefix  <path>`     설치 경로 (CMAKE_INSTALL_PREFIX)  
    * `--prefix ./external`

*  `--config  <type>`     CMake 구성 옵션 (Debug, Release 등)  
    * `--config Release`