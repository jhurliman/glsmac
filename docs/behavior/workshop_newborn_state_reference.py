"""Independent source-derived stored morale and immediate production flag transitions."""
def land_training(command=False,command_project=False,bio=False,bio_project=False,current_morale=0):
    training=2*int(command or command_project)+2*int(bio or bio_project)
    return training//2 if current_morale < -1 else training

def stored_morale(training,high_morale,first_prototype):
    return max(0,min(6,training+int(high_morale)+int(first_prototype)+1))

def production_flags(flags,base_flags,human,selected_owner):
    if human and (base_flags & 0x80000080) == 0x80000080:flags |= 0x200
    return flags | 0x2000, 0x10 if selected_owner else 0
