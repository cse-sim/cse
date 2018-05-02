require 'yaml'
require 'fileutils'
require_relative 'tables'
require_relative 'utils'

module Probes
  class BuildProbesAndCopyIntoManifest
    def initialize(config=nil)
      @disable = config.fetch('disable?', false)
      if !@disable
        @probes_dir = config.fetch('probes-build-dir')
        @out_dir = config.fetch('output-dir')
        @insert_after = config.fetch('insert-after-file')
        Utils::EnsureAllExist[[@probes_dir, @out_dir]]
        @probes_input = config.fetch('path-to-probes-input')
        @probes = YAML.load_file(@probes_input)
        @probes_in_one_file = config.fetch('probes-in-one-file?', false)
        if @probes_in_one_file
          @probes_paths = [[:all, File.join(@probes_dir, 'probes.md')]]
          write_probes_to_one_file(@probes_paths[0][1])
        else
          @probes_paths = []
          @probes.keys.sort_by {|k| k.downcase}.each do |k|
            k_dc = k.downcase
            @probes_paths << [k, File.join(@probes_dir, "probe_#{k_dc}.md")]
          end
          write_probes_to_files(@probes_paths)
        end
      end
    end
    # INPUTS:
    # k: String, the probe key into @probes
    # f: open file handle, note: this method does NOT open/close
    # RETURNS: nil
    # SIDE-EFFECT: Writes a probe table to the given file handle
    # Note: if @probes[k][:fields] is empty, returns without writing file
    def write_probe_table(k, f)
      table = [["Name", "Input?", "Runtime?", "Type", "Variability", "Description"]]
      flds = @probes[k][:fields]
      return if flds.empty?
      name = k
      array_txt = if @probes[k][:array] then "[1..]" else "" end
      title = "\\@#{name}#{array_txt}."
      owner = @probes[k][:owner]
      owner_txt = if owner == "--" then "" else " (owner: #{owner})" end
      tag = "\{#p_#{k.downcase}\}"
      f.write("## #{title}#{owner_txt} #{tag}\n\n")
      if @probes[k].include?(:description)
        f.write(@probes[k][:description] + "\n\n")
      end
      flds.each do |fld|
        table << [
          fld[:name],
          if fld[:input] then "X" else "--" end,
            if fld[:runtime] then "X" else "--" end,
            fld[:type],
            fld[:variability],
            fld.fetch(:description, "--")
        ]
      end
      f.write(Tables::WriteTable[ table, true ])
      f.write("\n\n\n")
    end
    # INPUTS:
    # - path: String, path to write probe tables to
    # RETURNS: nil
    # SIDE-EFFECT: Writes probe data to the given file-system location
    def write_probes_to_one_file(path)
      File.open(path, 'w') do |f|
        f.write("# Probe Definitions\n\n")
        @probes.keys.sort_by {|k| k.downcase}.each do |k|
          write_probe_table(k, f)
        end
      end
    end
    # INPUTS:
    # - keys_and_paths:: (Array (Tuple String String)), an array of 2-element
    #     arrays with first element the key into @probes and the second element the
    #     path corresponding to where that probe's info should be written
    # RETURN: nil
    # SIDE-EFFECT: Writes probe files to each table
    def write_probes_to_files(keys_and_paths)
      keys_and_paths.each do |k_p|
        k, p = k_p
        File.open(p, 'w') do |f|
          write_probe_table(k, f)
        end
      end
    end
    # INPUTS:
    # - manifest: (Array String), an array of paths
    # RETURNS: (Array String), the (possibly modified) manifest
    def call(manifest)
      if @disable
        manifest
      else
        new_manifest = []
        manifest.each do |path|
          bn = File.basename(path)
          out_path = File.join(@out_dir, bn)
          new_manifest << out_path
          Utils::CopyIfStale[path, out_path]
          if bn == @insert_after
            @probes_paths.each do |k_p|
              p = k_p[1]
              probes_out_path = File.join(@out_dir, File.basename(p))
              new_manifest << probes_out_path
              Utils::CopyIfStale[p, probes_out_path]
            end
          end
        end
        new_manifest
      end
    end
    # INPUTS:
    # - manifest: (Array String), an array of paths
    # RETURNS: (Array String), the (possibly modified) manifest
    # Note: alias for call(manifest)
    def [](manifest)
      call(manifest)
    end
  end
end
