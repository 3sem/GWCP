import sys

def generate_command(a_repeat, b_repeat, c_repeat):
    a_part = 'a' * a_repeat
    b_part = 'b' * b_repeat
    c_part = 'c' * c_repeat
    input_string = a_part + b_part + c_part
    command = f"./obparser -g ./tests/an_bn_cn -i {input_string} -o tree.dot"
    return command

try:
    times = int(sys.argv[1])
except:
    times = 1000
# Example usage with 'times' repetitions each:
print("TImes", times)
command_string = generate_command(times, times, times)
print(command_string)