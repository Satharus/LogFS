# LogFS
LogFS is a simple userspace filesystem that logs everything to a file. Made using C and LibFUSE high level API.


## Features
- List the contents of a directory (```ls```)
- Change the working directory (```cd```)
- Creating files and directories (```touch``` and ```mkdir```)
- Editing files (```nano``` or ```vim```)
- Deleting files and directories (```rm``` and ```rmdir```)
- Logs everything to a file, and give the user the ability to:
  - View the logs.
  - Reset the logs.
  - Mount the filesystem with logging disabled.
### To be added
- Support for links.
- Support for copying and moving(renaming).
- Changing permissions.
  
  
## Compilation

- Install libfuse3
  - You can install it from [here](https://github.com/libfuse/libfuse)

- Clone this repo
  - ```git clone https://github.com/Satharus/LogFS```

- Run ```./compile.sh```

## Testing

- Make a new directory to test LogFS
```mkdir New```

- Run LogFS
```./LogFS New```

- Test the filesystem like you'd use your usual filesystem from the CLI.

- Check the logs
```./LogFS --show-logs```

- Clear the logs
```./LogFS --reset-logs```
