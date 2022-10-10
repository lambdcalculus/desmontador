#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

#define word unsigned char
#define debug printf("debugando\n")

#define EI_NIDENT 16

#define Elf32_Word int
#define Elf32_Addr int
#define Elf32_Off int
#define Elf32_Half short

typedef struct {
        word            e_ident[EI_NIDENT];
        Elf32_Half      e_type;
        Elf32_Half      e_machine;
        Elf32_Word      e_version;
        Elf32_Addr      e_entry;
        Elf32_Off       e_phoff;
        Elf32_Off       e_shoff;
        Elf32_Word      e_flags;
        Elf32_Half      e_ehsize;
        Elf32_Half      e_phentsize;
        Elf32_Half      e_phnum;
        Elf32_Half      e_shentsize;
        Elf32_Half      e_shnum;
        Elf32_Half      e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	word		st_info;
	word		st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

typedef struct {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;


typedef struct {
	word  name[50];
	int   address;
	int   symtab_index;
} label;

typedef struct {
	word   name[50];
	int    address;
	int    numlabels;
	label  labels[50];
} section;

typedef struct {
	word*       buffer;
	Elf32_Ehdr* elfhdr;
	Elf32_Shdr* shtab;
	word*       shstrtab;
	Elf32_Sym*  symtab;
	word*       strtab;
	section     sections[50];
} Interface;

word* byteToHex(word decimal) {
	word digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	static word out[3] = "00";
	out[0] = digits[(decimal/16) % 16];
	out[1] = digits[decimal%16];
	
	return out;
}

word* intToHex(int decimal) {
	word digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	static word out[9] = "00000000";
	for (int i = 7; i > -1; i--){
		out[i] = digits[decimal%16];
		decimal = decimal/16;
	}
	return out;
}

word* stripZeroes(word* num_str) {
	static word out[9];
	int index = 0;
	while (index < 8){
		if (num_str[index] != '0'){
			break;
		}
		index++;
	}
	if (index == 8){
		out[0] = '0';
		out[1] = '\0';
	} else {
		for (int i = 0; i + index < 9; i++){
			out[i] = num_str[i + index];
		}
	}

	return out;
}

int strLen(word* str){
	int counter = 0;
	while (str[counter] != '\0') {
		counter++;
	}
	return counter;
}

int strCmp(word* str1, word* str2){
	if (strLen(str1) != strLen(str2)) {
		return 0;
	}
	int out = 1;
	for (int i = 0; i < strLen(str1); i++){
		if (str1[i] != str2[i]){
			out = 0;
		}
	}
	return out;
}

void copyString(word* str1, word* str2){
	for (int i = 0; i < strLen(str2)+1; i++) {
		str1[i]=str2[i];
	}
}

void copySections(section* a, section* b, int num){
	for (int i = 0; i < num; i++) {
		a[i] = b[i];
	}
}

void copyLabels(label* a, label* b, int num){
	for (int i = 0; i < num; i++) {
		a[i] = b[i];
	}
}

void sortSections(section* sections, int len){
	section sorted[len];
    //should be unsigned but idgaf
	int min_nextaddr = -1;
	int max_nextaddr = 0x0fffffff;
	int currmin_index;
	for (int i = 0; i < len; i++){
		//printf("Current min addr: 0x%x\n", min_nextaddr);
		//printf("Current max addr: 0x%x\n", max_nextaddr);
		//printf("Current min index: %d\n", currmin_index);
		for (int j = 0; j < len; j++){
			if ((sections[j].address > min_nextaddr) & (sections[j].address < max_nextaddr)){
				max_nextaddr = sections[j].address;
				currmin_index = j;
				//printf("NEW max addr: 0x%x\n", max_nextaddr);
				//printf("NEW min index: %d\n", currmin_index);
			}
		}
		sorted[i]    = sections[currmin_index];
		min_nextaddr = max_nextaddr;
		//printf("NEW min addr: 0x%x\n", min_nextaddr);
		max_nextaddr = 0x0ffffff;
	}
	copySections(sections, sorted, len);
}

void sortLabels(label* labels, int len){
	label sorted[len];
    //should be unsigned but idgaf
	int min_nextaddr = -1;
	int max_nextaddr = 0x0fffffff;
	int currmin_index;
	for (int i = 0; i < len; i++){
		//printf("Current min addr: 0x%x\n", min_nextaddr);
		//printf("Current max addr: 0x%x\n", max_nextaddr);
		//printf("Current min index: %d\n", currmin_index);
		for (int j = 0; j < len; j++){
			if ((labels[j].address > min_nextaddr) & (labels[j].address < max_nextaddr)){
				max_nextaddr = labels[j].address;
				currmin_index = j;
				//printf("NEW max addr: 0x%x\n", max_nextaddr);
				//printf("NEW min index: %d\n", currmin_index);
			}
		}
		sorted[i]    = labels[currmin_index];
		min_nextaddr = max_nextaddr;
		//printf("NEW min addr: 0x%x\n", min_nextaddr);
		max_nextaddr = 0x0ffffff;
	}
	copyLabels(labels, sorted, len);
}

void print(word* str){
	write(1, str, strLen(str));
}

short numDigits(short n){
	if (n==0){
		return 1;
	}
	int count = 0;

	while (n != 0){
		n = n/10;
		count++;
	}

	return count;
}	
//TODO: handle negatives
word* intToString(short n){
	static word out[5] = "0000";
	for (short digit = numDigits(n); digit > 0; digit--){
		out[digit-1] = n%10 + '0';
		n = n/10;
	}
	return out;
}

void printInt(unsigned short n){
	write(1, intToString(n), numDigits(n));
}

word* getSectionName(Interface* inter, int index){
	static word name[100];
	int offset = inter->shtab[index].sh_name;

	int i = 0;
	while (inter->shstrtab[offset+i] != '\0'){
		name[i] = inter->shstrtab[offset+i];
		i++;
	}
	name[i] = '\0';
	
	return name;
}

int getSymTabIndex(Interface* inter){
	for (int i = 0; i < inter->elfhdr->e_shnum; i++) {
		if (strCmp(getSectionName(inter, i), (word*) ".symtab")){
			return i;
		}
	}
	return 0;
}


int getStrTabIndex(Interface* inter){
	for (int i = 0; i < inter->elfhdr->e_shnum; i++) {
		if (strCmp(getSectionName(inter, i), (word*) ".strtab")){
			return i;
		}
	}
	return 0;
}

int getTextIndex(Interface* inter){
	for (int i = 0; i < inter->elfhdr->e_shnum; i++) {
		if (strCmp(getSectionName(inter, i), (word*) ".text")){
			return i;
		}
	}
	return 0;
}

int getNumLabels(Interface* inter, section* sec){
	int num = 0;
	for (int i = 0; i < inter->shtab[getSymTabIndex(inter)].sh_size/0x10; i++){
		if (strCmp(sec->name, getSectionName(inter, inter->symtab[i].st_shndx))){
			num++;
		}
	}
	return num;
}

label* getLabels(Interface* inter, section* sec){
	int num = 0;
	static label labels[50];
	for (int i = 0; i < inter->shtab[getSymTabIndex(inter)].sh_size/0x10; i++){
		if (strCmp(sec->name, getSectionName(inter, inter->symtab[i].st_shndx))){
			label lab;
			copyString(lab.name, &(inter->strtab[inter->symtab[i].st_name]));
			lab.address = inter->symtab[i].st_value;
			lab.symtab_index   = i;
			labels[num] = lab;
			num++;
		}
	}
	sortLabels(labels, num);
	return labels;
}

section* getSections(Interface* inter){	
	static section sections[50];

	for (int i = 0; i < inter->elfhdr->e_shnum; i++){
		copyString(sections[i].name, getSectionName(inter, i));
		sections[i].address   = inter->shtab[i].sh_offset;
		sections[i].numlabels  = getNumLabels(inter, &sections[i]);
		copyLabels(sections[i].labels, getLabels(inter, &sections[i]), sections[i].numlabels);
	}
	sortSections(sections, inter->elfhdr->e_shnum);
	return sections;
}

void printSectionTable(Interface* inter) {
	print((word*) "Sections:\nIdx Name Size VMA\n");
	for (int i = 0; i < inter->elfhdr->e_shnum; i++) {
		printInt((i));
		print((word*) " ");
		print(getSectionName(inter, i));
		print((word*) " ");
		print(intToHex(inter->shtab[i].sh_size));
		print((word*) " ");
		print(intToHex(inter->shtab[i].sh_addr));
		print((word*) "\n");
	}
}

void printSymTab(Interface* inter) {
	print((word*) "SYMBOL TABLE:\n");
	for (int i = 1; i < inter->shtab[getSymTabIndex(inter)].sh_size/0x10; i++){
		print(intToHex(inter->symtab[i].st_value)); print((word*) " ");
		if (((inter->symtab[i].st_info)>>4)&0x1){
			print((word*) "g");
		} else { 
			print((word*) "l");
		}
		print((word*) " ");
		if (inter->symtab[i].st_shndx == inter->symtab[0].st_shndx){
			print((word*) "*ABS*");
		} else {
		print(getSectionName(inter, inter->symtab[i].st_shndx));
		}
		print((word*) " ");
		print(intToHex(inter->symtab[i].st_size)); print((word*) " ");
		print(&(inter->strtab[inter->symtab[i].st_name])); print((word*) "\n");
	}
}

void disassembleProgram(Interface* inter){
	int text_index = getTextIndex(inter);
	print((word*)"Disassembly of section .text:\n\n");
	for (int i = 0; i < inter->sections[text_index].numlabels-1; i++){
		label lab = inter->sections[text_index].labels[i];
		int curr_address = lab.address;
		int buf_address = curr_address - inter->shtab[text_index].sh_addr + inter->shtab[text_index].sh_offset;
		print(intToHex(curr_address)); print((word*)" <"); print(lab.name); print((word*)">:\n");
		while (curr_address < inter->sections[text_index].labels[i+1].address){
			if (curr_address%4 == 0){
				print(stripZeroes(intToHex(curr_address))); print((word*) ":");
			}
			print((word*) " "); print(byteToHex(inter->buffer[buf_address]));
			if ((curr_address+1)%4== 0){ print((word*) "\n"); }
			curr_address++;
			buf_address++;
		}
		if ((curr_address+1)%4 != 0) {print((word*) "\n"); } else {print((word*) "\n\n");}
	}
	label lab = inter->sections[text_index].labels[inter->sections[text_index].numlabels-1];
	int curr_address = lab.address;
	int buf_address = curr_address - inter->shtab[text_index].sh_addr + inter->shtab[text_index].sh_offset;
	print(intToHex(curr_address)); print((word*)" <"); print(lab.name); print((word*)">:\n");
	while((buf_address - inter->shtab[text_index].sh_offset) < inter->shtab[text_index].sh_size) {
		if (curr_address%4 == 0){
			print(stripZeroes(intToHex(curr_address))); print((word*) ":");
		}
		print((word*) " "); print(byteToHex(inter->buffer[buf_address]));
		if ((curr_address+1)%4== 0){ print((word*) "\n"); }
		curr_address++;
		buf_address++;
	}
}

int main(int argc, char* argv[]){
	static word buf[100 * 1024];
	int fd = open(argv[2], O_RDONLY);

	read(fd, buf, 100 * 1024);

	Interface inter;
	inter.buffer   =                buf;
    inter.elfhdr   = (Elf32_Ehdr*) &buf[0];
	inter.shtab    = (Elf32_Shdr*) &buf[inter.elfhdr->e_shoff];
	inter.shstrtab =               &buf[inter.shtab[inter.elfhdr->e_shstrndx].sh_offset];
	inter.symtab   = (Elf32_Sym*)  &buf[inter.shtab[getSymTabIndex(&inter)].sh_offset];
	inter.strtab   = (word*)       &buf[inter.shtab[getStrTabIndex(&inter)].sh_offset];
	copySections(inter.sections, getSections(&inter), inter.elfhdr->e_shnum);
	
	print((word*) "\n");
	print((word*) argv[2]);
	print((word*) ": file format elf32-littleriscv\n\n");

	if (strCmp((word*) argv[1], (word*) "-h")) {
		printSectionTable(&inter);
	}
	else if (strCmp((word*) argv[1], (word*) "-t")) {
		printSymTab(&inter);
	}
	else if (strCmp((word*) argv[1], (word*) "-d")) {
		disassembleProgram(&inter);
	}
	
	close(fd);
}
