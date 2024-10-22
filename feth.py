# From env to header file, python utils with the aim to transform .env file in a .h file

import os
import re

def resolve_env_variable(value, env_variables):
    pattern = r'\$\{([^}]+)\}'
    matches = re.findall(pattern, value)
    for match in matches:
        resolved_value = os.getenv(match) or env_variables.get(match)
        if resolved_value:
            value = value.replace(f"${{{match}}}", resolved_value)
        else:
            print(f"Warning: Variable {match} not found.")
    
    return value

env_variables = {}
with open(".env") as file:
    for line in file:
        key, value = line.strip().split('=', 1)
        # Risolvi le variabili ${} nel valore
        value = resolve_env_variable(value, env_variables)
        env_variables[key] = value



with open("./components/env_var/include/env_var.h", "w") as header_file:
    header_file.write("#ifndef ENV_VARIABLES_H\n")
    header_file.write("#define ENV_VARIABLES_H\n\n")
    for key, value in env_variables.items():
        header_file.write(f'#define {key} "{value}"\n')
    header_file.write("\n#endif // ENV_VARIABLES_H\n")
