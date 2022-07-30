#include "bptree.h"
#include <vector>
#include <sys/time.h>
using namespace std;

/* * * * * * * * * * * * * * * * * * * *
	最初から用意されてるPrint系のやつ
* * * * * * * * * * * * * * * * * * * */

struct timeval cur_time(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

void print_tree_core(NODE *n) {
	printf("["); 
	for (int i = 0; i < n->nkey; i++) {
		if (!n->isLeaf) print_tree_core(n->chi[i]); //もらったのがinternal nodeならchildの要素で再帰させる
		printf("%d", n->key[i]);
		if (i != n->nkey-1 && n->isLeaf) putchar(' ');
	}
	if (!n->isLeaf) print_tree_core(n->chi[n->nkey]);
	printf("]");
}

void print_tree(NODE *node) {
	print_tree_core(node);
	printf("\n"); fflush(stdout);
}


/* * * * * * * * * * * * * * * * * * * *
	格納先探しに行くやつ
* * * * * * * * * * * * * * * * * * * */

NODE *find_leaf(NODE *node, int key) {
	int kid;

	if (node->isLeaf) return node; //nodeがリーフならnodeを返すよ
	for (kid = 0; kid < node->nkey; kid++) { //nkeyってnumber of keyのことか、nodeの中に入っているkeyの数？
		if (key < node->key[kid]) break; //どの下位ノードにいくかの選定
	}

	return find_leaf(node->chi[kid], key); //nodeがリーフじゃないなら再帰するよ
}

/* * * * * * * * * * * * * * * * * * * *
	allocまわり
* * * * * * * * * * * * * * * * * * * */

NODE *alloc_leaf(NODE *parent) {
	NODE *node;
    //要素数1、NODEのサイズ分のメモリ領域を確保する
	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
    //callocで全部0で初期化されてるらしいから、各要素に必要な情報を入れてる
	node->isLeaf = true;
	node->parent = parent;
	node->nkey = 0;

	return node;
}

TEMP *alloc_temp(NODE *node) {
    TEMP *temp;
    //仮置き用のメモリ領域を確保する
	if (!(temp = (TEMP *)calloc(1, sizeof(TEMP)))) ERR;
    //いろいろコピーする
	temp->isLeaf = node->isLeaf;
    for(int i = 0; i < node->nkey; i++) {
        temp->key[i] = node->key[i];
        temp->chi[i] = node->chi[i];
    }
    temp->nkey = node->nkey;
	
	if(node->isLeaf == false) {	//internal nodeのtempを作成するときはchildが1つ多いからその分
		temp->chi[temp->nkey] = node->chi[temp->nkey];
	}

    return temp;
}

/* * * * * * * * * * * * * * * * * * * *
	nodeの初期化とか
* * * * * * * * * * * * * * * * * * * */

void deleteLeaf(NODE *node) {
    for(int i = 0; i < node->nkey; i++) {
        node->key[i] = 0;
        node->chi[i] = 0;
    }
	if(node->isLeaf == false) {	//internal nodeならchildが1つ多いので
		node->chi[node->nkey] = 0;
	}
    node->nkey = 0;
}

/* * * * * * * * * * * * * * * * * * * *
	insertとか
* * * * * * * * * * * * * * * * * * * */

NODE *insert_in_leaf(NODE *leaf, int key, DATA *data) {
	int i;
	for (i = 0; i < leaf->nkey; i++) {  //入れれるところを線形探索して、後ろにずらす回数をiに入れる
		if (key < leaf->key[i]) break;
	}
	for (int j = leaf->nkey; j > i; j--) {
		leaf->chi[j] = leaf->chi[j-1] ;
		leaf->key[j] = leaf->key[j-1] ;
	} 
	/* CodeQuiz */
	leaf->key[i] = key;
	leaf->chi[i] = (NODE *)data;
	leaf->nkey++;

	return leaf;
}

NODE *insert_in_node(NODE *node, int key, NODE *childNode) {
	//insert(key, childNode) in node's parent just after *node
	int i;
	NODE *p = node->parent;
	for(i = 0; i < p->nkey+1; i++) {	//childはkeyより1つだけ多いからその分だけ
		if(p->chi[i] == node) break;
	}
	for(int j = p->nkey; j > i; j--) {
		p->chi[j+1] = p->chi[j];
		p->key[j] = p->key[j-1];
	}

	p->key[i] = key;
	p->chi[i+1] = childNode;
	p->nkey++;

	return node;
}

TEMP *insert_in_temp(TEMP *temp, int key, NODE *childNode) {
	int i;
	for (i = 0; i < temp->nkey; i++) {
		if (key < temp->key[i]) break;
	}
	if(temp->isLeaf == true) {	//leaf node用に作成したtempなら
		for (int j = temp->nkey; j > i; j--) {
			temp->chi[j] = temp->chi[j-1] ;
			temp->key[j] = temp->key[j-1] ;
		}
		temp->key[i] = key;
		temp->chi[i] = childNode;
	} else {	//internal node用に作成したtempなら(childの数がnkey+1だから)
		for (int j = temp->nkey; j > i; j--) {
			temp->chi[j+1] = temp->chi[j] ;
			temp->key[j] = temp->key[j-1] ;
		} 
		temp->key[i] = key;
		temp->chi[i+1] = childNode;
	}
	temp->nkey++;

	return temp;
}

int temp2node(NODE *node, NODE *newNode, TEMP *temp) {
	int i;
	if(temp->isLeaf == true) {	//leaf node用に作成したtempからleaf nodeに戻すなら
		for(i = 0; i < ceil(temp->nkey/2); i++) {
			node->key[i] = temp->key[i];
			node->chi[i] = temp->chi[i];
			node->nkey++;
		}
		for(i = 0; i < temp->nkey - ceil(temp->nkey/2); i++) {
			newNode->key[i] = temp->key[int(ceil(temp->nkey/2)) + i];
			newNode->chi[i] = temp->chi[int(ceil(temp->nkey/2)) + i];
			newNode->nkey++;
		}
	} else {	//internal node用に作成したtempからinternal nodeに戻すなら(leaf splitと勝手が違う)
		for(i = 0; i < ceil(temp->nkey/2); i++) {
			node->key[i] = temp->key[i];
			node->chi[i] = temp->chi[i];
			node->chi[i]->parent = node;	//親を変えないといけないよね
			node->nkey++;
		}
		node->chi[i] = temp->chi[i];	//iがインクリメント済みだからi+1じゃなくていいよ～～
		node->chi[i]->parent = node;	//親を変えないといけないよね
		for(i = 0; i < temp->nkey - (ceil(temp->nkey/2)+1); i++) {
			newNode->key[i] = temp->key[int(ceil(temp->nkey/2))+1 + i];
			newNode->chi[i] = temp->chi[int(ceil(temp->nkey/2))+1 + i];
			newNode->chi[i]->parent = newNode;	//親を変えないといけないよね
			newNode->nkey++;
		}
		newNode->chi[i] = temp->chi[int(ceil(temp->nkey/2))+1 + i];	//iはインクリメント済みだからi+1じゃなくてiでOK
		newNode->chi[i]->parent = newNode;	//親を変えないといけないよね

		return temp->key[int(ceil(temp->nkey/2))];
	}
	return 0;
}

NODE *insert_in_parent(NODE *node, int key, NODE* newNode) {
	NODE *newRoot;
	if(node == Root) {	//nodeがRootなら
		newRoot = alloc_leaf(NULL);
		//いい方法が思いつかないので強引に入れよう
		newRoot->chi[0] = node;
		newRoot->chi[1] = newNode;
		newRoot->key[0] = key;
		newRoot->nkey = 1;
		newRoot->isLeaf = false;
		Root = newRoot;
		//leafたちの親を変更する
		node->parent = Root;
		newNode->parent = Root;
		return Root;
	}
	//nodeがRootじゃないなら(internal nodeなら)
	NODE *p = node->parent;
	if(p->nkey < N-1) {	//nodeのparent nodeにスペースがあるなら
		insert_in_node(node, key, newNode);
	} else {			//parent nodeにスペースがないから分割する
		TEMP *temp;
		NODE *newInternal;
		cout << "- node split" << endl;
		temp = alloc_temp(p);
		insert_in_temp(temp, key, newNode);
		newInternal = alloc_leaf(p->parent);
		newInternal->isLeaf = false;	//使いまわしだからisLeafだけ変えとく
		deleteLeaf(p);
		int pkey = temp2node(p, newInternal, temp);	//再帰するとき用のparent nodeに登録するkey
		insert_in_parent(p, pkey, newInternal);
	}
	return node;
}

/* * * * * * * * * * * * * * * * * * * *
	Mainの処理
* * * * * * * * * * * * * * * * * * * */

void insert(int key, DATA *data) {
	NODE *leaf;
    NODE *newLeaf;
    TEMP *temp;

	if (Root == NULL) { 		//bptreeにな～んも要素がないときにやるよ
        cout << "- root is NULL" << endl;
		leaf = alloc_leaf(NULL);    			//親はNullになるよね(first insertだからそれはそう)
		Root = leaf;                			//tree構造のRootに作成したLeafを入れる
	} else {            		//bptreeに要素あるからleaf nodeを探しに行くよ
        cout << "- find leaf" << endl;
        leaf = find_leaf(Root, key);
    }
	if (leaf->nkey < (N-1)) { 	//leaf nodeに要素を入れられるんだったら
        cout << "- insert in leaf" << endl;
		insert_in_leaf(leaf, key, data);
	}
	else {                  	//leaf nodeに要素を入れられないんだったら分割するよ
        cout << "- leaf split" << endl;		
        temp = alloc_temp(leaf);				//0  tempをつくってとりあえず入れる
        insert_in_temp(temp, key, (NODE *)data);//1  insert_in_leaf流用できんから新しく作ったやつでinsertする
        newLeaf = alloc_leaf(leaf->parent);		//2  新しいleaf nodeを作成して、同じ親ノードを登録する
        leaf->chi[N] = newLeaf;					//3  leafのchi[N]をnewLeafにする(leaf内の線形探索で使える)
        deleteLeaf(leaf);						//4  leafのデータを消す
        temp2node(leaf, newLeaf, temp);			//5  tempからleafにデータを戻す
		insert_in_parent(leaf, newLeaf->key[0], newLeaf);        
		free(temp);
	}
}

void init_root(void) {
	Root = NULL;
}

//入力受け取るやつ
int interactive() {
    int key;

    std::cout << "Key: ";
    std::cin >> key;

    return key;
}

int main(int argc, char *argv[]) {
    struct timeval begin, end;

	init_root();

	printf("-----Insert-----\n");
	begin = cur_time();
    while (true) {
		int insert_key = interactive();
        insert(insert_key, NULL);
        print_tree(Root);
		cout << "--------------------------------------" << endl;
    }
	end = cur_time();

	return 0;
}
