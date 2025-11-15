# Implementation Plan

- [x] 1. Enhance Kernel with global instance and service validation





  - Add static Instance() method for global Kernel access
  - Implement RequireService<T>() method that throws if service missing
  - Add service validation and logging for missing services
  - Create helper macros (KERNEL, GET_SERVICE, REQUIRE_SERVICE)
  - _Requirements: 12.1, 12.2, 12.3_

- [x] 2. Remove simple forward declarations in Engine modules





  - [x] 2.1 Fix EditorRenderer.h forward declaration

    - Replace `class Model;` with `#include "Engine/Model/Core/Model.h"`
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3_
  

  - [x] 2.2 Fix EditorModule.h forward declaration

    - Replace `class Kernel;` with `#include "Engine/Kernel/Core/Kernel.h"`
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3_

- [x] 3. Refactor Player class to remove forward declarations





  - [x] 3.1 Remove forward declarations from Player.h


    - Replace `class PlayerCollision;` with `#include "Collision/PlayerCollision.h"`
    - Replace `class PlayerRenderable;` with `#include "PlayerRenderable.h"`
    - Replace `class IGameRenderable;` with proper include
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3_
  
  - [x] 3.2 Simplify Player constructor


    - Remove AudioManager parameter from constructor
    - Get AudioManager from Kernel in constructor body
    - Get CollisionManager from Kernel instead of passing it
    - Update all Player instantiation sites
    - _Requirements: 8.1, 8.4, 12.3_
  
  - [x] 3.3 Update Player to use Kernel for all services


    - Replace direct service passing with Kernel::GetService<T>()
    - Cache service pointers in member variables
    - Remove SetAudioManager() method
    - _Requirements: 12.1, 12.3_

- [x] 4. Eliminate PlayerManager and integrate into PlayerSystem









  - [x] 4.1 Move PlayerManager logic to PlayerSystem

    - Copy InitPlayer() logic to PlayerSystem::InitializePlayer()
    - Copy UpdatePlayerLogic() to PlayerSystem::UpdatePlayerLogic()
    - Update PlayerSystem to get services from Kernel
    - _Requirements: 3.1, 9.1, 11.3_
  
  - [x] 4.2 Remove PlayerManager class


    - Delete PlayerManager.h and PlayerManager.cpp
    - Remove PlayerManager from CMakeLists.txt
    - Remove all PlayerManager includes
    - _Requirements: 9.1, 11.2_
  


  - [x] 4.3 Update PlayerSystem.h to remove forward declarations








    - Replace `class Player;` with `#include "Game/Player/Player.h"`
    - Replace `class PlayerManager;` with nothing (removed)
    - Remove other forward declarations, add proper includes
    - _Requirements: 2.1, 2.3_

- [x] 5. Eliminate StateManager and integrate into PlayerSystem





  - [x] 5.1 Add state management to PlayerSystem


    - Add SavePlayerState() method to PlayerSystem
    - Add RestorePlayerState() method to PlayerSystem
    - Add HasSavedState() and GetSavedMapPath() methods
    - Add member variables for saved state (position, velocity, map path)
    - _Requirements: 3.1, 3.2, 11.3_
  
  - [x] 5.2 Remove StateManager class


    - Delete StateManager.h and StateManager.cpp
    - Remove StateManager from CMakeLists.txt
    - Update GameApplication to use PlayerSystem for state management
    - _Requirements: 9.1, 11.2_

- [x] 6. Eliminate MenuActionHandler and integrate into UIController







  - [x] 6.1 Move MenuActionHandler logic to UIController

    - Add HandleMenuActions() method to UIController
    - Add individual action handlers (HandleSinglePlayer, HandleResumeGame, etc.)
    - Implement game initialization coordination through Kernel services
    - _Requirements: 3.1, 11.3_
  

  - [x] 6.2 Remove MenuActionHandler class

    - Delete MenuActionHandler.h and MenuActionHandler.cpp
    - Remove MenuActionHandler from CMakeLists.txt
    - Update GameApplication to use UIController for menu actions
    - _Requirements: 9.1, 11.2_
  
  - [x] 6.3 Update UIController.h to remove forward declarations


    - Replace `class Menu;` with `#include "Game/Menu/Menu.h"`
    - Replace `class ConsoleManager;` with proper include
    - Replace `class Engine;` with `#include "Engine/Interfaces/IEngine.h"`
    - _Requirements: 2.1, 2.3_

- [x] 7. Refactor MapManager and integrate into MapSystem






  - [x] 7.1 Move MapManager logic to MapSystem


    - Copy all map loading logic to MapSystem
    - Copy collision initialization logic to MapSystem
    - Update MapSystem to get services from Kernel
    - _Requirements: 3.1, 11.3_
  


  - [x] 7.2 Remove MapManager class

    - Delete MapManager.h and MapManager.cpp
    - Remove MapManager from CMakeLists.txt
    - Update all MapManager references to use MapSystem


    - _Requirements: 9.1, 11.2_
  
  - [x] 7.3 Update MapSystem.h to remove forward declarations

    - Replace all forward declarations with proper includes
    - Organize includes in logical order
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3_

- [x] 8. Remove unnecessary single-implementation interfaces






  - [x] 8.1 Identify single-implementation interfaces


    - Scan codebase for interfaces with only one implementation
    - Document which interfaces to keep (for testing/mocking)
    - Create list of interfaces to remove
    - _Requirements: 9.1, 9.2_
  
  - [x] 8.2 Remove IPlayerProvider interface


    - Replace IPlayerProvider usage with direct PlayerSystem access
    - Delete IPlayerProvider.h
    - Update all code using IPlayerProvider
    - _Requirements: 9.1, 9.2_
  


  - [ ] 8.3 Remove IMapManagerProvider interface
    - Replace IMapManagerProvider usage with direct MapSystem access
    - Delete IMapManagerProvider.h
    - Update all code using IMapManagerProvider


    - _Requirements: 9.1, 9.2_
  
  - [ ] 8.4 Review and remove other unnecessary interfaces
    - Check each remaining interface for necessity
    - Remove interfaces that don't provide value
    - Keep interfaces needed for testing (IAudioManager, ICollisionManager)
    - _Requirements: 9.1, 9.2, 9.4_

- [x] 9. Simplify constructors with many parameters






  - [x] 9.1 Simplify MapSystem constructor

    - Create MapSystemConfig parameter object for configuration
    - Move service dependencies to Kernel-based access
    - Update constructor to take only MapSystemConfig
    - Update all MapSystem instantiation sites
    - _Requirements: 8.1, 8.2, 8.3, 8.4_
  
  - [x] 9.2 Simplify RenderingSystem constructor


    - Remove service parameters from constructor
    - Get all services from Kernel in Initialize() method
    - Update all RenderingSystem instantiation sites
    - _Requirements: 8.1, 8.4_
  


  - [x] 9.3 Simplify other constructors with 4+ parameters





    - Identify remaining constructors with many parameters
    - Apply appropriate pattern (Kernel/Builder/Parameter Object)
    - Update all instantiation sites
    - _Requirements: 8.1, 8.5_

- [x] 10. Remove remaining forward declarations in Systems





  - [x] 10.1 Fix RenderingSystem.h forward declarations


    - Replace all forward declarations with proper includes
    - Organize includes logically
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3_
  
  - [x] 10.2 Fix PlayerRenderable.h forward declaration


    - Replace `class Player;` with `#include "Player.h"`
    - Handle any circular dependency issues
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3, 3.1_

- [x] 11. Remove forward declarations in Menu and Console





  - [x] 11.1 Fix Menu.h forward declaration


    - Replace `class Kernel;` with `#include "Engine/Kernel/Core/Kernel.h"`
    - Update Menu to use Kernel::Instance() for global access
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3, 12.1_
  

  - [x] 11.2 Fix ConsoleManagerHelpers.h forward declaration

    - Replace `class Kernel;` with proper include
    - Update helper functions to use Kernel::Instance()
    - Verify compilation succeeds
    - _Requirements: 2.1, 2.3, 12.1_

- [-] 12. Standardize module structure across Engine



  - [x] 12.1 Audit current module structures
    - Document current folder structure for each module
    - Identify modules that don't follow Core/Interfaces/Utils pattern
    - Create standardization plan
    - _Requirements: 11.1, 11.4_
  
  - [ ] 12.2 Restructure Audio module








    - Ensure Core/ folder contains main implementation
    - Ensure Interfaces/ folder contains only necessary interfaces
    - Remove any unnecessary files
    - Update CMakeLists.txt

  
  - [ ] 12.3 Restructure Collision module
    - Organize files into Core/Interfaces/System/Structures
    - Consolidate duplicate code
    - Remove unnecessary files
    - Update CMakeLists.txt

  
  - [ ] 12.4 Restructure Model module
    - Organize files into Core/Interfaces/Utils
    - Move ModelAnalyzer to Utils/
    - Consolidate duplicate code
    - Update CMakeLists.txt
   
  
  - [ ] 12.5 Restructure remaining Engine modules
    - Apply Core/Interfaces/Utils pattern to all modules
    - Remove unnecessary files
    - Consolidate duplicate code
    - Update all CMakeLists.txt files

-

- [ ] 13. Standardize module structure in Game layer

















  - [-] 13.1 Restructure Player module

    - Organize into Core/ (Player.h/cpp), Interfaces/, Components/ (PlayerInput, PlayerMovement, etc.)
    - Remove unnecessary files
    - Update CMakeLists.txt
    
  
  - [ ] 13.2 Restructure Menu module



    - Organize into Core/, Console/, MapSelector/, Settings/
    - Consolidate duplicate code
    - Remove unnecessary files
    - Update CMakeLists.txt

  
  - [ ] 13.3 Clean up Managers folder
    - Remove all eliminated manager files
    - Keep only MapCollisionInitializer if still needed
    - Consider moving remaining files to appropriate Systems
    - Update CMakeLists.txt


- [ ]   - [ ] 14.1 Update GameApplication service registration
    - Consolidate all service registration into OnRegisterProjectServices()
    - Remove separate RegisterCoreKernelServices() and RegisterManagerKernelServices()
    - Ensure all services registered before Systems initialize
   
  
  - [ ] 14.2 Update Systems to register themselves
    - Each System registers itself as service in Initialize()
    - Remove manual System registration from GameApplication
    - Verify all Systems accessible through Kernel
  
  
  - [ ] 14.3 Remove all direct service passing
    - Find all places where services passed directly as parameters
    - Replace with Kernel::GetService<T>() calls
    - Verify no direct service passing remains
   14. Centralize all service registration through Kernel



- [ ] 15. Fix all identified bugs during refactoring




  - [ ] 15.1 Document all bugs found during refactoring
    - Create list of bugs discovered
    - Prioritize bugs by severity
    - Document expected vs actual behavior

  
  - [ ] 15.2 Fix critical bugs
    - Fix all crash bugs and data corruption bugs
    - Add comments explaining the fix
    - Add tests to prevent regression where possible

  
  - [ ] 15.3 Fix non-critical bugs
    - Fix all remaining bugs found during refactoring
    - Add comments explaining fixes
    - Add tests where appropriate


- [ ] 16. Verify complete removal of forward declarations
  - [ ] 16.1 Scan all header files for forward declarations
    - Use grep/search to find all `class ClassName;` patterns
    - Document any remaining forward declarations
    - Verify all are intentional (none should remain)

  
  - [ ] 16.2 Verify all headers compile independently
    - Create test that includes each header alone
    - Verify each header has all needed includes
    - Fix any missing includes


- [ ] 17. Update GameApplication to use simplified architecture

  - [ ] 17.1 Remove all manager member variables
    - Remove m_playerManager, m_stateManager, m_menuActionHandler
    - Remove m_mapManager if moved to MapSystem
    - Simplify GameApplication class

  
  - [ ] 17.2 Simplify GameApplication initialization
    - Remove InitializeManagers() method
    - Remove RegisterManagerKernelServices() method
    - Simplify OnPostInitialize() method

  
  - [ ] 17.3 Update GameApplication to use Systems
    - Replace manager calls with System calls through Kernel
    - Update HandleMenuActions() to use UIController
    - Update UpdatePlayerLogic() to use PlayerSystem
    - Update SaveGameState() to use PlayerSystem


- [ ] 18. Run comprehensive testing

  - [ ] 18.1 Run all unit tests
    - Execute all existing unit tests
    - Fix any failing tests
    - Verify all tests pass
  
  
  - [ ] 18.2 Run integration tests
    - Execute all integration tests
    - Test system coordination through Kernel
    - Fix any failing tests

  
  - [ ] 18.3 Manual gameplay testing
    - Test all menu actions work correctly
    - Test player movement and controls
    - Test map loading and transitions
    - Test save/restore functionality
    - Test all game features
 

- [ ] 19. Performance and compilation verification

  - [ ] 19.1 Measure compilation time
    - Clean build before refactoring (baseline)
    - Clean build after refactoring
    - Verify increase is less than 15%
    - _Requirements: 13.4_
  
  - [ ] 19.2 Performance testing



    - Measure frame rate in typical gameplay
    - Compare with pre-refactoring performance
    - Verify no significant degradation
    - _Requirements: 13.4_
  
  - [ ] 19.3 Memory leak testing
    - Run game with memory profiler
    - Check for memory leaks from service changes
    - Fix any leaks found
    - _Requirements: 13.4_

- [ ] 20. Documentation and cleanup

  - [ ] 20.1 Update architecture documentation
    - Document new Kernel-centric architecture
    - Document module structure standards
    - Document service registration patterns
    - _Requirements: 11.4_
  
  - [ ] 20.2 Update code comments
    - Add comments explaining Kernel usage
    - Update comments that reference removed managers
    - Add comments for complex refactorings
    - _Requirements: 10.2_
  
  - [ ] 20.3 Create migration guide
    - Document what changed and why
    - Provide examples of old vs new patterns
    - Document how to add new services
    - Document how to create new Systems
    - _Requirements: 11.4_
  
  - [ ] 20.4 Final code review and cleanup
    - Review all changed files
    - Remove any commented-out code
    - Fix any code style issues
    - Ensure consistent formatting
    - _Requirements: 13.5_
