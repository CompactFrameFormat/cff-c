# Custom rake tasks for the project
require 'fileutils'
require 'yaml'

# Load the project configuration
CONFIG = YAML.load(File.read('project.yml'))

# Create convenient accessors for the configuration sections
CONFIG.each_key do |k|
  eval "#{k.to_s.upcase} = CONFIG[:#{k}]"
end

namespace :format do
  desc "Run clang-format on all source and test files"
  task :all do
    puts "Running clang-format on all source and test files..."
    
    # Use paths from project.yml
    source_patterns = PATHS[:source].map { |path| File.join(path, '**', '*.{c,h}') }
    test_patterns = PATHS[:test].select { |path| !path.start_with?('-:') }
                                .map { |path| path.sub(/^\+:/, '') }
                                .map { |path| File.join(path, '**', '*.{c,h}') }
    example_patterns = ['example/**/*.{c,h}']
    
    # Exclude support directories and unity files
    source_files = FileList[*source_patterns]
    test_files = FileList[*test_patterns].exclude('test/support/**/*', 'test/unity.*')
    example_files = FileList[*example_patterns].exclude('example/build/**/*')
    all_files = source_files + test_files + example_files
    
    if all_files.empty?
      puts "No source or test files found to format."
      return
    end
    
    # Check if clang-format is available
    if system('clang-format --version > nul 2>&1')
      puts "Found clang-format, formatting #{all_files.length} files..."
      
      formatted_count = 0
      all_files.each do |file|
        puts "  Formatting: #{file}"
        if system("clang-format -i \"#{file}\"")
          formatted_count += 1
        else
          puts "    ERROR: Failed to format #{file}"
        end
      end
      
      puts "Successfully formatted #{formatted_count}/#{all_files.length} files."
    else
      puts "ERROR: clang-format not found in PATH. Please install clang-format."
      exit 1
    end
  end
  
  desc "Check if files are properly formatted (dry run)"
  task :check do
    puts "Checking if all source and test files are properly formatted..."
    
    # Use paths from project.yml
    source_patterns = PATHS[:source].map { |path| File.join(path, '**', '*.{c,h}') }
    test_patterns = PATHS[:test].select { |path| !path.start_with?('-:') }
                                .map { |path| path.sub(/^\+:/, '') }
                                .map { |path| File.join(path, '**', '*.{c,h}') }
    example_patterns = ['example/**/*.{c,h}']
    
    # Exclude support directories and unity files
    source_files = FileList[*source_patterns]
    test_files = FileList[*test_patterns].exclude('test/support/**/*', 'test/unity.*')
    example_files = FileList[*example_patterns].exclude('example/build/**/*')
    all_files = source_files + test_files + example_files
    
    if all_files.empty?
      puts "No source or test files found to check."
      return
    end
    
    # Check if clang-format is available
    if system('clang-format --version > nul 2>&1')
      puts "Found clang-format, checking #{all_files.length} files..."
      
      unformatted_files = []
      all_files.each do |file|
        # Use clang-format --dry-run to check if file would be changed
        result = `clang-format --dry-run --Werror "#{file}" 2>&1`
        if $?.exitstatus != 0
          unformatted_files << file
          puts "  NEEDS FORMATTING: #{file}"
        else
          puts "  OK: #{file}"
        end
      end
      
      if unformatted_files.empty?
        puts "All files are properly formatted!"
      else
        puts "\nFound #{unformatted_files.length} files that need formatting:"
        unformatted_files.each { |file| puts "  - #{file}" }
        puts "\nRun 'ceedling format:all' to fix formatting issues."
        exit 1
      end
    else
      puts "ERROR: clang-format not found in PATH. Please install clang-format."
      exit 1
    end
  end
end

# Add a top-level format task that defaults to format:all
desc "Run clang-format on all source and test files"
task :format => 'format:all' 