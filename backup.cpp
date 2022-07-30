#include "bptree.h"
#include <vector>
#include <sys/time.h>
#include <chrono>
using namespace std;
#define num 1000000

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
	check用のやつ
* * * * * * * * * * * * * * * * * * * */

void shuffleData(int *data, int size) {
	int i, target, tmp;
	for(i = size - 1; i > 0; i--) {
		target = rand() % i;
		tmp = data[target];
		data[target] = data[i];
		data[i] = tmp;
	}
}

void createData(int *array, int option) {
	cout << "createArray ";
	if(option == 0) {			//昇順で作成
		cout << "ASC" << endl;
		for(int i = 0; i < num; i++) array[i] = i;	//昇順
	} else if(option == 1) {	//降順で作成
		cout << "DESC" << endl;
		for(int i = num; i > 0; i--) array[i] = i;	//降順
	} else {					//randomで作成
		cout << "rand" << endl;
		for(int i = 0; i < num; i++) array[i] = i;
		shuffleData(array, num);
	}
}

bool find_data(NODE *node, int key) {
	for(int i = 0; i < node->nkey; i++) {
		if(node->key[i] == key) {
			return true;
		}
	}
	return false;
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
	//cout << "=insert in node=" << endl;
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

	//cout << "=[end]insert in node=" << endl;

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
		//cout << "- node split" << endl;
		temp = alloc_temp(p);
		insert_in_temp(temp, key, newNode);
		newInternal = alloc_leaf(p->parent);
		newInternal->isLeaf = false;	//使いまわしだからisLeafだけ変えとく
		deleteLeaf(p);	//p
		int pkey = temp2node(p, newInternal, temp);	//再帰するとき用のparent nodeに登録するkey
		insert_in_parent(p, pkey, newInternal);
		free(temp);
	}
	return node;
}

/* * * * * * * * * * * * * * * * * * * *
	deleteの処理
* * * * * * * * * * * * * * * * * * * */

int count_child(NODE *node);

template <typename T>
void delete_from_node(NODE *node, int key, T pointer) {
	int i, j;
	for(i = 0; i < node->nkey; i++) {	//keyの削除
		if(key == node->key[i]) break;
	}
	for(i; i < node->nkey-1; i++) {
		node->key[i] = node->key[i+1];
	}
	for(j = 0; j < node->nkey+1; j++) {	//chiの消去
		if(pointer == node->chi[j]) break;
	}
	for(j; j < node->nkey; j++) {
		node->chi[j] = node->chi[j+1];
	}
	//おしりのデータが重複or消去対象になるから消す
	node->key[node->nkey-1] = 0;	//nullptrのほうが良いのか？
	node->chi[node->nkey] = 0;
	node->nkey -= 1;
}

NODE *find_sibling(NODE *node, NODE *chi) {
	int i;
	//Let N′ be the previous or next child of parent(N)
	for(i = 0; i < node->nkey+1; i++) {
		if(node->chi[i] == chi) break;
	}
	if(i == 0) {
		return node->chi[i+1];
	} else {
		return node->chi[i-1];
	}
}

int find_parentKey(NODE *Pnode, NODE *chiNode, NODE *sibNode) {
	int i;
	for(i = 0; i < Pnode->nkey; i++) {
		if(Pnode->chi[i] == chiNode || Pnode->chi[i] == sibNode) break;
	}

	return Pnode->key[i];
}

bool find_nodeOrder(NODE *Pnode, NODE *chiNode, NODE *sibNode) {
	int i;
	for(i = 0; i < Pnode->nkey; i++) {
		if(Pnode->chi[i] == chiNode || Pnode->chi[i] == sibNode) break;
	}
	if(Pnode->chi[i] == chiNode) {
		return true;
	} else {
		return false;
	}
}

void move_node2sibNode(NODE *node, NODE *sibNode) {
	for(int i = 0; i < node->nkey; i++) {
		sibNode->key[sibNode->nkey + i] = node->key[i];
		sibNode->chi[sibNode->nkey + i] = node->chi[i];
		if(sibNode->isLeaf == false) {
			sibNode->chi[sibNode->nkey + i]->parent = sibNode;	//親変えないとね
		}
	}
	sibNode->nkey += node->nkey;
	if(node->isLeaf == false) {
		sibNode->chi[sibNode->nkey] = node->chi[node->nkey];
		sibNode->chi[sibNode->nkey]->parent = sibNode;
	}
}

int count_child(NODE *node) {
	int ans = 0;
	for(int i = 0; i < node->nkey+1; i++) {
		if(node->chi[i] != 0) ans++;
	}
	return ans;
}

template <typename T>
void delete_entry(NODE *node, int key, T pointer) {
    NODE *sibNode, *temp;
	int parentKey, m;
	delete_from_node(node, key, pointer);
	if(node->parent == NULL && count_child(node) == 1) {	//[key,chi]=(1,2)からある(key,chi)を消すと(0,1)になるから
		Root = node->chi[0]; //(key,chi)を消して左に詰めたらchi[0]がRootの対象になるのでは？
		Root->parent = NULL;	//Rootになるなら親をNullにしないと
		//free(node->chi);
		//free(node->parent);
		//cout << "delete_node -> " << node << endl;
		free(node);
	} else if(node->parent != NULL && (node->isLeaf == true && node->nkey < (int)ceil((N-1)/2.0) || node->isLeaf == false && node->nkey+1 < (int)ceil(N/2))) {	//leafとnon-leafで条件が違うよ
		sibNode = find_sibling(node->parent, node);	//結合先(N')の選定
		parentKey = find_parentKey(node->parent, node, sibNode);	//(K')
		if(node->nkey + sibNode->nkey < N && count_child(node) + count_child(sibNode) < N+1) {
			if(find_nodeOrder(node->parent, node, sibNode)) {	//sibNode->Nodeの順番にしたいらしい
				temp = sibNode;
				sibNode = node;
				node = temp;
			}
			if(node->isLeaf == false) {
				sibNode->key[sibNode->nkey] = parentKey;
				sibNode->nkey++;
				move_node2sibNode(node, sibNode);
			} else {	//nodeがleaf nodeなら
				move_node2sibNode(node, sibNode);
				sibNode->parent = node->parent;
			}
			delete_entry(node->parent, parentKey, node);
			//free(node->chi);
			//free(node->parent);
			//cout << "delete_node -> " << node << endl;
			free(node);
		} else {	//NとN'が一つにまとめられないからN'から借りて再配布する
			if(find_nodeOrder(node->parent, node, sibNode) == false) {
				if(node->isLeaf == false) {
					m = sibNode->nkey;

					//(N'->pointer[m], K')をNの先頭に入れて、他のを1個右シフトする
					for(int i = node->nkey; i > 0; i--) {	//keyの移動
						node->key[i] = node->key[i-1];
					}
					for(int i = count_child(node); i > 0; i--) {	//chiの移動
						node->chi[i] = node->chi[i-1];
					}
					node->key[0] = parentKey;
					node->chi[0] = sibNode->chi[m];
					node->chi[0]->parent = node;	//親変えないとね
					node->nkey++;

					//node->ParentのparentKeyを、sibNode->key[m-1]に置き換える
					for(int i = 0; i < node->parent->nkey; i++) {	
						if(node->parent->key[i] == parentKey) {
							node->parent->key[i] = sibNode->key[m-1];
							break;
						}
					}
					sibNode->key[m-1] = 0;	//消去の処理を後に入れないとめんどくね？
					sibNode->chi[m] = 0;
					sibNode->nkey -= 1;
				} else {	//nodeがleafなら
					m = sibNode->nkey-1;
					insert_in_leaf(node, sibNode->key[m], (DATA*)sibNode->chi[m]);
					for(int i = 0; i < node->parent->nkey; i++) {
						if(node->parent->key[i] == parentKey) {
							node->parent->key[i] = sibNode->key[m];
						}
					}
					sibNode->key[m] = 0;	//こっちもじゃね？
					sibNode->chi[m] = 0;
					sibNode->nkey -= 1;
				}
			} else {
				if(sibNode->isLeaf == false) {

					node->key[node->nkey] = parentKey;
					node->chi[node->nkey+1] = sibNode->chi[0];
					node->chi[node->nkey+1]->parent = node;	//親変えないとね
					node->nkey++;

					for(int i = 0; i < sibNode->parent->nkey; i++) {
						if(sibNode->parent->key[i] == parentKey) {
							sibNode->parent->key[i] = sibNode->key[0];	////N(左)のケツをParentに上書きするなら、先にN'key[0]で上書きしちゃったほうが楽じゃね？
							break;
						}
					}
					delete_from_node(sibNode, sibNode->key[0], sibNode->chi[0]);
				} else {

					node->key[node->nkey] = sibNode->key[0];
					node->chi[node->nkey] = sibNode->chi[0];
					node->nkey++;


					for(int i = 0; i < sibNode->parent->nkey; i++) {
						if(sibNode->parent->key[i] == parentKey) {
							sibNode->parent->key[i] = sibNode->key[0];	////N(左)のケツをParentに上書きするなら、先にN'key[0]で上書きしちゃったほうが楽じゃね？
							break;
						}
					}
					delete_from_node(sibNode, sibNode->key[0], sibNode->chi[0]);
				}
			}
		}
	}
}


void delete_data(int key, NODE *pointer) {
	NODE *leaf;
	// 1.消したいデータの場所を探しに行く
	leaf = find_leaf(Root, key);
	delete_entry(leaf, key, pointer);
}


/* * * * * * * * * * * * * * * * * * * *
	Mainの処理
* * * * * * * * * * * * * * * * * * * */

void insert(int key, DATA *data) {
	NODE *leaf;
    NODE *newLeaf;
    TEMP *temp;

	if (Root == NULL) { 		//bptreeにな～んも要素がないときにやるよ
        //cout << "- root is NULL" << endl;
		leaf = alloc_leaf(NULL);    			//親はNullになるよね(first insertだからそれはそう)
		Root = leaf;                			//tree構造のRootに作成したLeafを入れる
	} else {            		//bptreeに要素あるからleaf nodeを探しに行くよ
        //cout << "- find leaf" << endl;
        leaf = find_leaf(Root, key);
    }
	if (leaf->nkey < (N-1)) { 	//leaf nodeに要素を入れられるんだったら
        //cout << "- insert in leaf" << endl;
		insert_in_leaf(leaf, key, data);
	}
	else {                  	//leaf nodeに要素を入れられないんだったら分割するよ
        //cout << "- leaf split" << endl;		
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


// int main(int argc, char *argv[]) {
//     struct timeval begin, end;

// 	init_root();

// 	int d[10];
// 	int ans = 0;
// 	NODE *r;

// 	//arrayつくるよ(0:ASC, 1:DESC, 2:rand)
// 	createData(d, 2);

// 	//dataいれるよ
// 	for(int i = 0; i < 10; i++) insert(d[i], NULL);

// 	print_tree(Root);

// 	createData(d, 2);

// 	//dataけすよ
// 	for(int i = 0; i < 10; i++) {
// 		cout << "====================" << endl;
// 		cout << "key = " << d[i] << endl;
// 		delete_data(d[i], NULL);
// 		print_tree(Root);
// 		r = Root;
// 		cout << "Root = ["; for(int i=0;i<Root->nkey;i++){cout << Root->key[i] << ((i != Root->nkey-1) ? " " : "") ;} cout << "]" << endl;
// 	}

// 	end = cur_time();

// 	return 0;
// }






// 72時間用のやつだけどdeleteでFreeできてないからいったん保留

int main(int argc, char *argv[]) {
    struct timeval begin, end;
	auto start = std::chrono::system_clock::now();

	init_root();

	int d[num];
	int data;
	int current, current_m = 0;
	int data_item = num;
	char command[128];

	//arrayつくるよ(0:ASC, 1:DESC, 2:rand)
	createData(d, 2);

	//dataいれるよ
	for(int i = 0; i < num; i++) insert(d[i], NULL);

	while(1) {
		data = rand() % num;
		if(find_data(find_leaf(Root, data),data)) {
			delete_data(data, NULL);
			data_item--;
		} else {
			insert(data, NULL);
			data_item++;
		}

		//currentのオーバーフロー対策
		if(current < num) {
			current++;
		} else {
			current_m++;
			current = 0;
			//cout << current_m << endl;
		}

		if(current == 0) {
			if(current_m % 100 == 0) {
				auto end = std::chrono::system_clock::now();
				auto dur = end - start;        // 要した時間を計算
				auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
				cout << "====================" << endl;
				cout << "time = ";
				printf("%d:%d:%d",(int)sec/3600, (int)sec%3600/60, (int)sec%60);
				cout << " | current = " << current_m << "M | data_item = " << data_item << endl;
				sprintf(command, "grep VmRSS /proc/%d/status", getpid());
				system(command);
			} else {
				cout <<"wip: " << current_m%100 << "%" << flush;
				cout << "\r";
				//cout << "\r";
			}
		}

		// //sprintf(command, "grep VmRSS /proc/%d/status", getpid());
		// sprintf(command, "free -h");
		// system(command);

		// cout << "start_delete" << endl;
		// for(int i = 0; i < num; i++) {
		// 	delete_data(i, NULL);
		// }
		// cout << "end_delete" << endl;

		// //sprintf(command, "grep VmRSS /proc/%d/status", getpid());
		// sprintf(command, "free -h");
		// system(command);

		// cout << "start_insert" << endl;
		// for(int i = 0; i < num; i++) {
		// 	insert(i, NULL);
		// }
		// cout << "end_insert" << endl;

	}


	end = cur_time();

	return 0;
}
