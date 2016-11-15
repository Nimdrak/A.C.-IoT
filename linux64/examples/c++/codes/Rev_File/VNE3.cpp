#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>


#define MAXPATH 20 //패스의 최대 길이 
#define MAXWINQ 40 //윈도우 큐 스토리지 사이즈 
#define VNNUM 300         // 버츄어 네트워크 수 
#define TWIN 10    //한 윈도우 길이
#define MAXDELAY 3         // 버츄어 네트워크 최대 딜레이 횟수 
#define MAXVNODENUM 10     // 버츄어 네트워크 최대 노드 수 
#define SNODENUM 70      // 물리 네트워크 노드 수 
#define MAXSLINKNUM 500     // 물리 네트워크 최대 링크 수
#define KEY 7              // 케이 
#define EPS 7              // 엡실론 
#define ALPHA 2             // 가상 네트워크 revenue계산할때 노드자원과 링크자원의 가중치
#define CONFMODE 2 //1 : 1hop, 2 : 2hop, 3 : 거리모델 
#define CONFLINKD 30 //컨플릭트 그래프가 거리모델일 때, 간섭하는 링크의 거리
#define POIMEAN 5 //한 타임 윈도우안에 들어오는 가상 네트워크 요청 수가 평균이 POIMEAN인 poisson 분포를 따름
#define EXPMEAN 10 //한 가상 네트워크 요청의 지속 기간이 평균이 EXPMEAN인 exponential random variable개의 타임 윈도우가 됨.
 
#define randomize() srand((unsigned)time(NULL))
#define random(num) (rand()%(num))
 
class queue;
class Snet;
class Snode;
class Sedge;
class Vnet;
class Vnode;
class Vedge;

Snet* SN;// 서브넷 네트워크 - 동적 생성을 위해 포인터로! 
Snet* bufSN; // 버프 서브넷 네트워크 
Snet* tempSN;
Vnet** VNs;
Vnet** bufVNs; //버프 가상 네트워크들. 비교 알고리즘을 돌리기 위해 필요.
int conflictG[MAXSLINKNUM][MAXSLINKNUM];
FILE *f;
FILE *f1;
FILE *result;


class Sedge{//물리링크
      //edge data
      public :
             int node1;//엣지에 연결된 노드
             int node2;//엣지에 연결된 노드
             int weight;//웨이트! 밴드위쓰
             int edgeID;//엣지 번호
             int filledW;//채워진 웨이트(임베딩 중에 늘어남)
};

class Snode{//물리노드
      //node data
      public :
             int weight;//노드 웨이트! 씨퓌유
             int* links;//노드의 링크들. 여러개 링크가 있을테니 엣지수 받아서 배열로 저장(엣지 아이디 저장)
             int edgenum;//링크 수
             int nodeID;//노드 번호
             int filledW;//채워진 양
             int x;//좌표 x값
             int y;//좌표 y값
             
             Snode(int egnum){
                       links = (int *)malloc(sizeof(int)*egnum);
                       edgenum = egnum;
             }
             
};

class Vedge{//가상링크
      //edge data
      public :
             int node1;//엣지에 연결된 노드
             int node2;//엣지에 연결된 노드
             int weight;//웨이트! 밴드위스
             int edgeID;//엣지 번호
             int embeddedpath[MAXPATH];//임베딩 된 물리네트워크의 패쓰
};

class Vnode{//가상노드
      //node data
      public :
             int weight;//웨이트! 씨퓌유
             int* links;//링크들
             int edgenum;//링크 수
             int nodeID;//노드 번호
             int embeddedSNID;//임베딩된 물리 노드 번호
             
             Vnode(int egnum){
                       links = (int *)malloc(sizeof(int)*egnum);
                       edgenum = egnum;
             }
             
};

class Snet{//물리 네트워크
      //Subnet and Virnet
      public :
             Snode** nodes;//노드들  Snode* 타입의 포인터들의 배열
             Sedge** edges;//링크들
             int nodenum;//노드수
             int edgenum;//링크수
             int** map;//2차 배열 그래프, adjacent matrix. int*-map[i] 포인터들의 배열.
             
             Snet(int ndnum, int egnum){//이니셜라이져(콘스트럭터)
                     //initialize
                     int i, j;
                     nodes = (Snode **)malloc(sizeof(Snode*)*ndnum);
                     edges = (Sedge **)malloc(sizeof(Sedge*)*egnum);
                     nodenum = ndnum;
                     edgenum = egnum;
                     map = (int **)malloc(sizeof(int*)*ndnum);
                     for( i=0;i<ndnum;i++){
                          map[i]=(int *)malloc(sizeof(int)*ndnum);
                          for( j=0;j<ndnum;j++){
                               map[i][j]=0;
                          }
                     }
             }
};

class Vnet{//가상네트워크
      //Subnet and Virnet
      public :
             int startT;//요청이 들어온 시간
             int RqT;//요구된 지속시간
             int endT;//임베딩 했을때 계산된다. 임베딩이 끝나는 시간, 요청이 억셉된 시간은 endT-RqT
             int delayed;//임베딩 수락여부 검사에서 현재까지 탈락된 횟수
             int nodenum;//노드 수
             int edgenum;// 링크 수
             int isEmbedded;//임베딩 되었을 때 1
             Vnode** nodes;//노드들 
             Vedge** edges;//링크들
             int** map;//2차배열 그래프, adjacent matrix. int*-map[i] 포인터들의 배열.
             
             Vnet(int ndnum, int egnum, int sT, int rT){
                     //initialize
                     int i, j;
                     nodes = (Vnode **)malloc(sizeof(Vnode*)*ndnum);
                     edges = (Vedge **)malloc(sizeof(Vedge*)*egnum);
                     map = (int **)malloc(sizeof(int *)*ndnum);
                     for( i=0;i<ndnum;i++){
                          map[i] = (int *)malloc(sizeof(int)*ndnum);
                          for( j=0;j<ndnum;j++){
                               map[i][j] = 0;
                          }
                     }
                     nodenum = ndnum;
                     edgenum = egnum;
                     startT = sT;
                     RqT = rT;
             }
};

class queue{
                    
public :
    int* storage; //큐 자체
    int Qsize; //큐의 크기
    int top;
    int bot;
              
    queue(int quesize){
	int i;
	storage = (int *)malloc(sizeof(int)*quesize);
	for( i=0;i<quesize;i++){
	    storage[i] = -1;
	}
	Qsize = quesize;
	top=0;
	bot=0;
    }
             
    void enqueue(int ev){
	//TODO:: enqueue
	storage[top]=ev;
	top=(top+1)%Qsize;
                            
    }
             
    int dequeue(){
	//TODO:: dequeue
	int temp;
	if(top==bot)return -1;
	temp = storage[bot];
	storage[bot]=-1;
	bot=(bot+1)%Qsize;
	return temp;
                    
    }
             
    void sort(int key){//key = 0 : 가상 네트워크의 링크 개수 , key = 1 : 나가는 시간 
	//sort
	int i,j;
	int* temp = (int *)malloc(sizeof(int)*Qsize);
	int swap;
	memcpy(temp,storage,sizeof(int)*Qsize);
                  
             
	if( bot >= top){
	    for(i = bot; i < Qsize; i++)storage[i-bot] = temp[i];
	    for(i = 0; i < top; i++)storage[Qsize-bot+i] = temp[i];
	    top = (Qsize-bot+i)%Qsize;
	    for(i = top; i< Qsize; i++)storage[i]=-1;
	    bot = 0;
	}

	if( key==0){  //가상 네트워크의 링크 개수에 따라 오름차순으로 정렬
	    for( j=bot+1; j<top; j++){
		for(i=j-1; (i>=bot) &&(VNs[storage[i]]->edgenum > VNs[storage[i+1]]->edgenum);i--){   
		    swap = storage[i+1];
		    storage[i+1]=storage[i];
		    storage[i] = swap;
		}
			              
	    }
	}
	else if(key==1){ //임베딩이 종료되는 시점에 대해 오름차순으로 정렬
	    for( j=bot+1; j<top; j++){
		for(i=j-1; (i>=bot) &&(VNs[storage[i]]->endT > VNs[storage[i+1]]->endT);i--){   
		    swap = storage[i+1];
		    storage[i+1]=storage[i];
		    storage[i] = swap;
		}
			               
	    }
	}
	if(key==0)
	{
	    fprintf(f,"WindowQ Sort Result - %d, %d : ",top, bot);
	    for( i=0;i<Qsize;i++){
		j=0;
		if(storage[i]!=-1)j = VNs[storage[i]]->edgenum;
		fprintf(f," (%d- %d- %d)",i,storage[i], j);
	    }
	    fprintf(f,"\n");
	}

	else if(key==1)
	{
	    fprintf(f,"EmbeddedQ Sort Result - %d, %d : ",top, bot);
	    for( i=0;i<Qsize;i++){
		j=0;
		if(storage[i]!=-1)j = VNs[storage[i]]->endT;
		fprintf(f," (%d- %d- %d)",i,storage[i], j);
	    }
	    fprintf(f,"\n");
	}
    }             
	int isEmpty(){
	    //isEmpty 1: empty, 0: not empty
	    if((storage[bot]==-1)&&(top==bot))return 1;
	    else return 0;
	}
};

queue WindowQ(MAXWINQ);//윈도우 큐. 임베딩 해야될 가상네트워크 큐
queue WindowQ1(MAXWINQ); //비교 알고리즘 돌릴때의 윈도우 큐
queue EmbeddedQ(VNNUM);//임베딩 되어 있는 가상네트워크 큐-원래 알고리즘 대로 임베딩
queue EmbeddedQ1(VNNUM);//비교 알고리즘 대로 임베딩했을때, 임베딩 된 가상 네트워크 큐


void SPath(int node1, int node2, int* resultarray);
int getSlink(int node1, int node2);
void BFS(int innet, int root, int** inout);
void NodeMapOrder(int VID, int* output);
void SNodeOrder(int* output);
int Embed(int VID, int NID, int* embedArr);
void NEmbedTo(int VID, int from, int to);
void EEmbedTo(int VID, int from, int* to);
void Unmap(int VID);
int isFeasible(Snet FSN);
void SNcpy(Snet* to, Snet* from);
void VNscpy(Vnet** to, Vnet** from);
void mkConflictG();
void MakeSN(int nodenum, int xrange, int yrange, int cvmin, int cvmax, int nwmin, int nwmax, int lwmin, int lwmax);
void MakeVNs(int vnnum, int nwmin, int nwmax, int lwmin, int lwmax);
int RecSpath(int pathlength, int nowlen, int nownode, int dest, int* result);
int poisson(int poimean);
int exponential(int expmean);
int VNRevenue(Vnet* VN);
int CompEmbed(int VID, int* embedArr);


int main(){
    int i,j,k;
    int virnum=VNNUM; //생성할 가상 네트워크 개수 
    int time=0;
    int temp;
    int temp2;
    int mapVnet;   //임베딩할 가상 네트워크
    int mapped;
    int winnum;  //현재가 몇번 째 윈도우?
    int TotRevenue=0;
    int TotRevenue1=0;
    double AvRevenue;
    double AvRevenue1;
    int while1, while2;
    int VOrderArr[MAXVNODENUM];
    int SOrderArr[SNODENUM];
    int EmdArr[SNODENUM];

    f = fopen("log.txt","wt");
    result = fopen("result.txt","wt");


    //초기화
    MakeSN(SNODENUM, 150, 150, 10, 18, 50, 150, 50, 150);
    MakeVNs(virnum, 10, 30, 10, 30);
    mkConflictG();

    bufVNs = (Vnet**)malloc(sizeof(Vnet*)*virnum);
    for(i=0;i<virnum;i++)
	bufVNs[i]=(Vnet*)malloc(sizeof(Vnet));
			    
    VNscpy(bufVNs, VNs);
    
    printf("initialize done\n");
    //T.O.D.O.:: 파일 입출력 -> Substrate net, Virtual net(Virnet에 추가로 시간 할당)

    fprintf(f,"SN map :\n");
 
    for( i=0;i<SN->nodenum;i++){
	for( j=0;j<SN->nodenum;j++){
	     fprintf(f,"%d ", SN->map[i][j]);
	}
	fprintf(f,"\n");
    }
    fprintf(f,"\n");

    for( i=0;i<SN->nodenum;i++){
	fprintf(f,"snode %d : %d\n",i,SN->nodes[i]->weight);
    }
    fprintf(f, "\n");
    for( i=0;i<SN->edgenum;i++){
	fprintf(f,"sedge %d : %d\n",i,SN->edges[i]->weight);
    }

    fprintf(f,"VN map :\n");
     
    for( i=0;i<virnum;i++){

	fprintf(f,"VN[%d] :\n",i);
	for( j=0;j<VNs[i]->nodenum;j++){
	    for( k=0;k<VNs[i]->nodenum;k++){
		fprintf(f,"%d ", VNs[i]->map[j][k]);
	    }
	    fprintf(f,"\n");
	}
	fprintf(f,"\n");
    }
     
    for( i=0;i<virnum;i++){
	fprintf(f,"VN[%d] :\n",i);

        for( j=0;j<VNs[i]->nodenum;j++){
	    fprintf(f,"%d vnode %d : %d\n",i,j,VNs[i]->nodes[j]->weight);
	}
	fprintf(f,"\n");
	for( j=0;j<VNs[i]->edgenum;j++){
	    fprintf(f,"%d vedge %d : %d\n",i,j,VNs[i]->edges[j]->weight);
	}
    }

    fprintf(result, " Our algorithm result \n");

    winnum = 0;
    while1 = 1;
    while(while1){
	fprintf(f,"Time Window %d : %d\n", winnum, time);
	time+=TWIN;
            
            
	//Enqueue 발생 Virtual network
	for( i=0;i<virnum;i++){
	    temp = VNs[i]->startT;
	    if(( temp>(time-TWIN))&&(temp<=time)){
		WindowQ.enqueue(i);
	    }
	}
	fprintf(f,"%d : Enqueue done\n",winnum);
	//while Break condition
	if(WindowQ.isEmpty()==1&&EmbeddedQ.isEmpty()==1){
	    fprintf(f,"%d : Queue Empty -> checking left VNs\n",winnum);
	    //남은 가상네트워크가 있는지 확인 
	    while1 = 0;//break;
	    for( i=0;i<virnum;i++){
		if((VNs[i]->startT)>time)while1 = 1;
	    }
	    if(while1 == 0){
		continue;
		fprintf(f,"%d : Embedding done\n",winnum);
	    }
	}
            
	//Window handling
            
	//지난 윈도우 동안 빠져나간 network 처리
	EmbeddedQ.sort(1);//오름차순 : 나가는 시간
	fprintf(f,"%d : Handling 'going out' VNs\n",winnum);
	while(1){
	    temp = EmbeddedQ.dequeue();
	    if( temp == -1){
		fprintf(f,"%d : No going out VNs\n",winnum);
		break;
	    }
	    if( VNs[temp]->endT<=time){
		//매핑 해제
		fprintf(f,"%d : %d is going out\n", winnum, temp);
		Unmap(temp);
	    }
	    else {
		EmbeddedQ.enqueue(temp);
		break;
	    }
	} 
	//dequeue -> mapping 
	temp2 = -1;//처음으로 안들어간 버넷 번호 저장 (브레이크 조건용) 
	
        //Sort
	WindowQ.sort(0);//오름차순 : 링크 개수 
	fprintf(f,"%d : Start to Embedding\n",winnum);


	while2 = 1;
	while( while2){
	    mapVnet = WindowQ.dequeue(); //mapVnet째 가상 네트워크 요청을 임베딩 해본다. 
	    if(mapVnet == -1){
		fprintf(f,"%d : WindowQ empty\n",winnum);
		while2 = 0;
		continue;
	    }
	    else if(mapVnet==temp2) // 현재 윈도우에서 임베딩 실패해서 다시 넣은 가상 네트워크가 mapvnet이므로 이것을 다시 임베딩 해볼 필요없음.
	    {
		while2=0;//break;
		WindowQ.enqueue(mapVnet);
		continue;
	    }
		
	    //mapping
	    mapped = 0;

            NodeMapOrder(mapVnet, VOrderArr);
	    fprintf(f,"%d : Node mapping order of VN%d : ",winnum, mapVnet);
	    for( i=0;i<MAXVNODENUM;i++){
		fprintf(f," %d", VOrderArr[i]);
	    }
	    fprintf(f,"\n");
	    //가장 큰 노드 임베드
	    //S노드들 정렬
	    SNodeOrder(SOrderArr);
	    fprintf(f,"%d : Node order of SN : ",winnum);
	    for( i=0;i<SN->nodenum;i++){
		fprintf(f," %d", SOrderArr[i]);
	    }
	    fprintf(f,"\n");
                   
	    for( j=0;j<KEY;j++){
		fprintf(f,"%d : Trying Embedding - %d'th time\n",winnum, j);
		//for 조건 : S노드를 돌며 
		//가장 큰 S노드 부터 temp에 ID기록
		SNcpy(bufSN,SN);
	
		temp = SOrderArr[j]; // check!!!

		for( i=0; i<SNODENUM;i++)EmdArr[i]=-1;
	

		if(VNs[mapVnet]->nodes[VOrderArr[0]]->weight > SN->nodes[SOrderArr[j]]->weight-SN->nodes[SOrderArr[j]]->filledW) //물리 노드의 자원보유량이 부족한 경우
		    continue;

		NEmbedTo(mapVnet, VOrderArr[0], temp);
	
		EmdArr[temp]=VOrderArr[0];
		fprintf(f,"%d : Init done\n",winnum);
                        
		for( i=1;i<VNs[mapVnet]->nodenum;i++){//모두 임베드 할때 까지
		    fprintf(f,"%d : Node Embed VN: %d, Nodenum: %d \n",winnum, mapVnet, VOrderArr[i]);
		    temp = Embed(mapVnet, VOrderArr[i], EmdArr);
		    fprintf(f,"%d : Node Embedding Result is %d\n",winnum, temp);
		    if( temp == 0){
			mapped = 2;
			i = VNs[mapVnet]->nodenum + 10;
		    }
		}
		//if(매핑불가)mapped = 2 
                               
		//feasible? -> for loop 탈출 
		if( isFeasible(*bufSN)==1&&mapped!=2){
		    j=KEY+10;
		    SNcpy(SN,bufSN);
		    VNs[mapVnet]->isEmbedded = 1;
		    mapped = 1;
		    
		}
                               
	    }
                   
	    if( mapped == 1){
		//빠져나갈 시간 계산
		VNs[mapVnet]->endT=VNs[mapVnet]->RqT+time;
               	EmbeddedQ.enqueue(mapVnet);
		
	    }

	    else{
		for(i=0;i<VNs[mapVnet]->nodenum;i++)  //노드 매핑 결과가 남아있을수도 있으므로 리셋
		    VNs[mapVnet]->nodes[i]->embeddedSNID =-1;
		
		for(i=0;i<VNs[mapVnet]->edgenum;i++)  //링크 매핑 결과가 남아있을수도 있으므로 리셋
		    for(j=0;j<MAXPATH;j++)
			VNs[mapVnet]->edges[i]->embeddedpath[j] =-1;
		 
		if( VNs[mapVnet]->delayed < MAXDELAY){
		    VNs[mapVnet]->delayed++;
		    WindowQ.enqueue(mapVnet);
		    		    
		    if(temp2 == -1)temp2 = mapVnet; //temp2에 이미 임베딩해봤는데 실패한 가상 네트워크의 번호를 저장한다.
		}
	    }

	    if(WindowQ.isEmpty()==1)while2 = 0;//break;
	}
	
	for(i=0;i<EmbeddedQ.Qsize;i++)
	{
	    if(EmbeddedQ.storage[i]!=-1)
		TotRevenue += VNRevenue(VNs[EmbeddedQ.storage[i]]);
	}
	
	AvRevenue = (double)TotRevenue/winnum;

	fprintf(result," winnum : %d - TotRevenue : %d,   AvRevenue : %lf \n", winnum, TotRevenue, AvRevenue);

	winnum++;
            
    }
    fprintf(f,"End of Embedding\n");
    for(i=1;i<200;i++)
    {
	fprintf(result,"-");
    }
    fclose(f);
 
/********************************************************************************


                 비교 대상 알고리즘 구현 


**********************************************************************************/
    
    //물리 네트워크 초기화
    for(i=0;i<SN->nodenum;i++)
	SN->nodes[i]->filledW=0;
    
    for(i=0;i<SN->edgenum;i++)
	SN->edges[i]->filledW=0;
    
    //가상 네트워크 초기화
    VNscpy(VNs,bufVNs);
    
    time=0;
    winnum=0;  //현재가 몇번 째 윈도우?
    
    f1 = fopen("log1.txt","wt");
    
    fprintf(result, " Comparison algorithm result \n");

    while1 = 1;
    while(while1){
	fprintf(f1,"Time Window %d : %d\n", winnum, time);
	time+=TWIN;
                    
	//Enqueue 발생 Virtual network
	for( i=0;i<virnum;i++){
	    temp = VNs[i]->startT;
	    if(( temp>(time-TWIN))&&(temp<=time)){
		WindowQ1.enqueue(i);
	    }
	}
	fprintf(f1,"%d : Enqueue done\n",winnum);
	//while Break condition
	if(WindowQ1.isEmpty()==1&&EmbeddedQ1.isEmpty()==1){
	    fprintf(f1,"%d : Queue Empty -> checking left VNs\n",winnum);
	    //남은 가상네트워크가 있는지 확인 
	    while1 = 0;//break;
	    for( i=0;i<virnum;i++){
		if((VNs[i]->startT)>time)while1 = 1;
	    }
	    if(while1 == 0){
		continue;
		fprintf(f1,"%d : Embedding done\n",winnum);
	    }
	}
            
	//Window handling
            
	//지난 윈도우 동안 빠져나간 network 처리
	EmbeddedQ1.sort(1);//오름차순 : 나가는 시간
	fprintf(f1,"%d : Handling 'going out' VNs\n",winnum);
	while(1){
	    temp = EmbeddedQ1.dequeue();
	    if( temp == -1){
		fprintf(f1,"%d : No going out VNs\n",winnum);
		break;
	    }
	    if( VNs[temp]->endT<=time){
		//매핑 해제
		fprintf(f1,"%d : %d is going out\n", winnum, temp);
		Unmap(temp);
	    }
	    else {
		EmbeddedQ1.enqueue(temp);
		break;
	    }
	} 
	//dequeue -> mapping 
	temp = -1;//처음으로 안들어간 버넷 번호 저장 (브레이크 조건용) 
	
///////////////////////////////////////////////////////////////////////////////
        
	fprintf(f1,"%d : Start to Embedding\n",winnum);

	while2 = 1;
	while( while2){
	    mapVnet = WindowQ1.dequeue();
	    if(mapVnet == -1){
		fprintf(f1,"%d : WindowQ1 empty\n",winnum);
		while2 = 0;
		continue;
	    }
	    else if(mapVnet==temp)while2=0;//break;
	   
	   
	    //가상 네트워크  mapVnet을 임베딩 한다. 
	    for(i=0; i<SNODENUM;i++)EmdArr[i]=-1;
	    
	    mapped = CompEmbed(mapVnet,EmdArr);

	    if(mapped)
	    {
		VNs[mapVnet]->endT=VNs[mapVnet]->RqT+time;
               	EmbeddedQ1.enqueue(mapVnet);
	    }
	   
	    else{
		for(i=0;i<VNs[mapVnet]->nodenum;i++)  //노드 매핑 결과가 남아있을수도 있으므로 리셋
		    VNs[mapVnet]->nodes[i]->embeddedSNID =-1;
		
		for(i=0;i<VNs[mapVnet]->edgenum;i++)  //링크 매핑 결과가 남아있을수도 있으므로 리셋
		    for(j=0;j<MAXPATH;j++)
			VNs[mapVnet]->edges[i]->embeddedpath[j] =-1;
		 
		if( VNs[mapVnet]->delayed < MAXDELAY){
		    VNs[mapVnet]->delayed++;
		    WindowQ1.enqueue(mapVnet);
		    		    
		    if(temp == -1)temp = mapVnet;
		}
	    }

	    if(WindowQ1.isEmpty()==1)while2 = 0;//break;

	}
	
	for(i=0;i<EmbeddedQ1.Qsize;i++)
	{
	    if(EmbeddedQ1.storage[i]!=-1)
		TotRevenue1 += VNRevenue(VNs[EmbeddedQ1.storage[i]]);
	}
	
	AvRevenue1 = (double)TotRevenue1/winnum;

	fprintf(result," winnum : %d - TotRevenue1 : %d,   AvRevenue1 : %lf \n", winnum, TotRevenue1, AvRevenue1);

	winnum++;
            
    }
    fprintf(f1,"End of Embedding\n");
    fclose(f1);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
    printf("Simulation End \n");
    fclose(result);

    for(i=0;i<virnum;i++)
    {
	free(bufVNs[i]);
    }
    free(bufVNs); 
    
}

//Functions


//Spath : 쇼티스트 패쓰 찾는다. node1, node2 사이의 쇼티스트 패쓰 찾는다. resultarray에 패쓰 저장한다.
void SPath(int node1, int node2, int* resultarray){
      
    int top;
    int i,j,k,p,q;
    int** BFSArr;
    int pathlength = -1;
    int nodenum = SN->nodenum;
     
    BFSArr = (int **)malloc(sizeof(int*)*nodenum);
    for( i=0;i<nodenum;i++){
	BFSArr[i] = (int*)malloc(sizeof(int)*nodenum);
	for( j=0;j<nodenum;j++){
	    BFSArr[i][j]=-1;
	}
    }

     for( j=0;j<MAXPATH;j++)resultarray[j]=-1;
  
    if(node1 == node2){
	pathlength = 0;
	return;
    }
  
    BFS(-1, node1, BFSArr);

    for( i=0;i<nodenum;i++){
	for( j=0;j<nodenum;j++){
	    if( BFSArr[i][j]==node2)pathlength = i;
	    BFSArr[i][j]=0;
	}
    }

    top = 0;
   //지금까지는 path의 길이만 찾았으므로 path자체를 찾자.
    RecSpath(pathlength, 0, node1, node2, resultarray);
 
    for( i=pathlength;i<MAXPATH;i++){
	resultarray[i] = -1;
    }
     
    for( i=0;i<nodenum;i++){
	free(BFSArr[i]);
    }
    free(BFSArr);
}

int RecSpath(int pathlength, int nowlen, int nownode, int dest, int* result){ //shortest path의 길이를 알고있을때 그 path를 찾는 함수.
    int i,j,k;

    if(pathlength == nowlen){
	return -1;
    }
    for( i=0;i<SN->nodenum;i++){
	if(SN->map[nownode][i]==1){
	    result[nowlen] = getSlink(nownode, i);
	    nowlen++;
	    if(i==dest){
		return 1;
	    }
	    else{
		j = RecSpath(pathlength, nowlen, i, dest, result);
		if(j==-1){
		    nowlen--;
		    result[nowlen] = 0;
		}
		else if(j==1){
		    return 1;
		}
	    }
	}
    }
    return -1;
}



//getSlink 노드 두개를 인자로 받아 그것을 잇는 링크의 아이디 반환. 없으면 -1반환
int getSlink(int node1, int node2){
    int i;
    for( i=0;i<SN->edgenum;i++){
	if( ((SN->edges[i]->node1==node1)&&(SN->edges[i]->node2==node2))||((SN->edges[i]->node1==node2)&&(SN->edges[i]->node2==node1))){
	    return i;
	}
    }
    return -1;
}
// innet이 -1이면 물리네트워크를, -1이 아니면 그 번호에 해당하는 가상네트워크를 BFS. root는 루트임. inout에 결과 저장.

void BFS(int innet, int root, int** inout){//BFS inout2차 배열에 홉별로 저장 inout[hop][k]
    int** mappt; //BFS할 그래프
    int* que; //큐
    int* checker; //큐에 그 노드가 들어왔었는지(들어왔다 나간것도 포함)를 체크
    int top, bot;
    int endflag; //루트로 부터 hop 홉 만큼 떨어진 노드들중 마지막 노드, 큐에서 이 다음 노드는 hop+1홉 만큼 떨어진 노드들
    int hop;
    int i,j,k;
    int temp;
    int nodenum;
     
    if( innet == -1){
	mappt = SN->map;
	nodenum = SN->nodenum;
    }
    else{
	mappt = VNs[innet]->map;
	nodenum = VNs[innet]->nodenum;
    }
     
    que = (int*)malloc(nodenum*sizeof(int));
    checker = (int*)malloc(nodenum*sizeof(int));
     
    top=0;
    bot=0;
    for( i=0;i<nodenum;i++){
	que[i]=-1;
	checker[i] = 0;
    }
     
    que[top]=root;
    checker[root]=1;
    if(root>=nodenum)printf("error!\n");
    top++;
    inout[0][0]=root;
    hop=1;
    endflag = root;
    k=0;
     
    while( top!=bot){
	//dequeue
	temp = que[bot];
	if(temp == -1||bot<0||bot>=nodenum)printf("error!\n");
	que[bot]=-1;
	bot=(bot+1)%nodenum;
	j=0;
	if(temp==endflag)j=1;
	for( i=0;i<nodenum;i++){
	   
	    if( mappt[temp][i]==1&&checker[i]==0){
		if(hop>=nodenum||k>=nodenum||top<0||top>=nodenum)printf("error!\n");
		inout[hop][k]=i;
		k++;
		que[top]=i;
		top = (top+1)%nodenum;
		checker[i]=1;
		if( j==1)endflag = i;
	    }
	}
            
            
            
	if( j==1){
	    hop++;
	    k=0;
	}

    }
    free(que);
    free(checker);


}

//가상 노드 매핑 순서. 임베딩 할 노드 순서를 찾아서 output에 저장. VID는 가상네트워크 아이디!
void NodeMapOrder(int VID, int* output){
    int BiggestNode;
    int BiggestValue;
    int hop;
    int len;
    int** BFSArr;
    int* costTemp;
    int i,j,k;
    int nodenum = VNs[VID]->nodenum;
    int flag;
     
    BFSArr = (int **)malloc(sizeof(int*)*nodenum);
    costTemp = (int *)malloc(sizeof(int)*nodenum);

    for( i=0;i<nodenum;i++){
	BFSArr[i] = (int *)malloc(sizeof(int)*nodenum);
	costTemp[i] = 0;
	for( j=0;j<nodenum;j++)BFSArr[i][j]=-1;
    }

    for( i=0;i<MAXVNODENUM;i++){
	output[i]=-1;
    }

    BiggestNode=0;
    BiggestValue=0;
    k=0;
    for( i=0;i<nodenum;i++){
	k=0;
	for( j=0;j<VNs[VID]->nodes[i]->edgenum;j++){
	    k+=VNs[VID]->edges[(VNs[VID]->nodes[i]->links[j])]->weight;
	}
	k *= VNs[VID]->nodes[i]->weight;
	costTemp[i]=k; //가상 노드 i의 자원 요구량
	if( k>BiggestValue){
	    BiggestValue = k;
	    BiggestNode = i;
	}
    }
    BFS(VID, BiggestNode, BFSArr);
    fprintf(f,"BFS Result (root : %d) :\n", BiggestNode);
    for( i=0;i<nodenum;i++){
	for( j=0;j<nodenum;j++){
	    fprintf(f,"%d ",BFSArr[i][j]);
	}
	fprintf(f,"\n");
    }

    hop=0;
    len=0;
    flag=0;
    for( i=0;i<nodenum;i++){
	//sort BFSArr 같은 홉 만큼 떨어진 노드들에 대해 COST를 기준으로 다시 정렬
	for( j=1;j<nodenum;j++){
	    if(BFSArr[i][j]==-1){
		j=nodenum;
		continue;
	    }
	    else{
		if( costTemp[BFSArr[i][j-1]]<costTemp[BFSArr[i][j]]){
		    for( k=j;k>0&&costTemp[BFSArr[i][k-1]]<costTemp[BFSArr[i][k]];k--){
			BiggestNode = BFSArr[i][k-1];
			BFSArr[i][k-1]=BFSArr[i][k];
			BFSArr[i][k]=BiggestNode;
		    }
                    
		}
	    }
	}
    }
    for( i=0;i<nodenum;i++){
	if( BFSArr[hop][len]==-1){
	    hop++;
	    len=0;
	}
	output[i] = BFSArr[hop][len];
	len++;
    }
     
    free(costTemp);
    for( i=0;i<nodenum;i++){
	free(BFSArr[i]);
    }
    free(BFSArr);
}

//에스노드오더. 물리노드 정렬하여 output에 저장. 자원남은거 총량이 제일 큰 노드부터 정렬
void SNodeOrder(int* output){
    //S 노드 정렬(큰거부터) 
    int* costTemp;
    int i,j,k;
    int nodenum = SN->nodenum;
    int temp;
    costTemp = (int *)malloc(sizeof(int)*nodenum);

    for( i=0;i<nodenum;i++){
	k=0;
	for( j=0;j<SN->nodes[i]->edgenum;j++){
	    k+=(SN->edges[(SN->nodes[i]->links[j])]->weight-SN->edges[(SN->nodes[i]->links[j])]->filledW);
	}
	k *= (SN->nodes[i]->weight-SN->nodes[i]->filledW);
	costTemp[i]=k;
	output[i]=i;
    }
 
    for( j=1;j<nodenum;j++){
	if( costTemp[output[j-1]]<costTemp[output[j]]){
	    for( k=j;k>0&&costTemp[output[k-1]]<costTemp[output[k]];k--){
		temp = output[k-1];
		output[k-1]=output[k];
		output[k]=temp;
	    }
                    
	}
    }

    free(costTemp);
}
 
//임베드!.가상 노드하나 임베딩 하고, 그 관련된 가상 링크들까지 bufSN에 임베딩 한다. VID는 가상네트워크 아이디, NID는 임베딩할 가상 노드 아이디. embedArr[k]는 물리 노드 k에 어떤 가상 노드가 임베딩 되었는지 알려줌, 1이면 성공, 0이면 실패. 
int Embed(int VID, int NID, int* embedArr){
    //노드 임베딩 -> 링크까지 (bufSN)
    int i,j,k,p;
    int *can;  //가상노드를 임베딩할 물리노드 후보군 
    int nodenum = bufSN->nodenum;
    int pathtemp[MAXPATH];
    int temp;
    int* costTemp;
    int* neighbor; // 가상노드들 tempnbr이 임베딩된 물리노드들
    int* tempnbr; // NID의 이웃중 이미 임베딩된 가상 노드들 
    int neibnum = VNs[VID]->nodes[NID]->edgenum; //최종적으로 neighbor 개수가 됨 
    int** BFSArr;

    BFSArr = (int **)malloc(sizeof(int*)*nodenum);

    for( i=0;i<nodenum;i++){
	BFSArr[i] = (int *)malloc(sizeof(int)*nodenum);
	for( j=0;j<nodenum;j++)BFSArr[i][j]=-1;
    }
    costTemp = (int *)malloc(sizeof(int)*nodenum);
    can = (int *)malloc(sizeof(int)*nodenum);
    tempnbr = (int *)malloc(sizeof(int)*neibnum);
    temp = 0;
    for( i=0;i<neibnum;i++){ //tempnbr 찾음
	if( VNs[VID]->edges[VNs[VID]->nodes[NID]->links[i]]->node1 == NID)
	    tempnbr[i] = VNs[VID]->edges[VNs[VID]->nodes[NID]->links[i]]->node2;
	else  tempnbr[i] = VNs[VID]->edges[VNs[VID]->nodes[NID]->links[i]]->node1;
         
	k=0;
	temp++;
	for( j=0;j<nodenum;j++){
	    if(tempnbr[i] == embedArr[j])k=1;
	}
	if(k==0){
	    tempnbr[i]=-1;
	    temp--;
	}
    }

    neighbor = (int *)malloc(sizeof(int)*temp);
    j=0;
    for( i=0;i<neibnum;i++){ //neighbor찾음
	if( tempnbr[i]!=-1){
	    for( k=0;k<nodenum;k++){
		if( embedArr[k]==tempnbr[i]){
		    neighbor[j] = k;
		    j++;
		}
	    }
	}
    }
    
    neibnum = temp;
    
    //find can
    j=0;
    k=1;
    for( i=0;i<nodenum;i++){
	if( (bufSN->nodes[i]->weight-bufSN->nodes[i]->filledW)>VNs[VID]->nodes[NID]->weight&&embedArr[i]<0){
	    can[j] = i;
	    j++;
	}
	else{
	    can[nodenum-k]=-1;
	    k++;
	}
    }
    
    ////can을 정렬한다. 먼저 자원에 대해 sort한 후에 홉 거리에 따라 최종적으로 sort
    for( i=0;i<nodenum;i++){ 
	k=0;
	for( j=0;j<bufSN->nodes[i]->edgenum;j++){
	    k+=(bufSN->edges[(bufSN->nodes[i]->links[j])]->weight-bufSN->edges[(bufSN->nodes[i]->links[j])]->filledW);
	}
	k *= (SN->nodes[i]->weight-SN->nodes[i]->filledW);
	costTemp[i]=k;
    }
    for( j=1;j<nodenum&&can[j]>=0;j++){
	if( costTemp[can[j-1]]<costTemp[can[j]]){
	    for( k=j;k>0&&costTemp[can[k-1]]<costTemp[can[k]];k--){
		temp = can[k-1];
		can[k-1]=can[k];
		can[k]=temp;
	    }
                    
	}
    }
    for( i=0;i<nodenum;i++){ //물리 노드 i에 대해서 홉거리 계산..
	for( j=0; j<nodenum;j++){
	    for( k=0;k<nodenum;k++){
		BFSArr[j][k] = -1;
	    }
	}
        BFS(-1,i,BFSArr);
	temp = 0;
	for( j=0;j<neibnum;j++){
	    for( k=0;k<nodenum;k++){
		for( p=0;p<nodenum;p++){
		    if( BFSArr[k][p]==neighbor[j]){
			temp+=k;
			p=nodenum;
			k=nodenum;
		    }
		}
	    }
	}
	costTemp[i] = temp;
    }

    for( j=1;j<nodenum&&can[j]>=0;j++){
	if( costTemp[can[j-1]]>costTemp[can[j]]){
	    for( k=j;k>0&&costTemp[can[k-1]]>costTemp[can[k]];k--){
		temp = can[k-1];
		can[k-1]=can[k];
		can[k]=temp;
	    }
                    
	}
    }
     
    temp = 0;    
    for( i=0;i<EPS&&can[i]>=0;i++){ //-1이 아닌 can[i]의 개수가 EPS보다 작을 수 있다.
	//try embedding
	SNcpy(tempSN,bufSN); //tempSN에 NID를 임베딩 하기전의 상태(bufSN)을 저장한다.

	NEmbedTo(VID, NID, can[i]);

	for( j=0;j<neibnum;j++){ 
	    k=0;
	    for( p=0;p<VNs[VID]->edgenum;p++){ //neighbor[j]에 해당되는 가상 노드와 연결된 링크를 찾는다. 
		if( (VNs[VID]->edges[p]->node1==NID&&VNs[VID]->edges[p]->node2==embedArr[neighbor[j]])||(VNs[VID]->edges[p]->node2==NID&&VNs[VID]->edges[p]->node1==embedArr[neighbor[j]])){
		    k=p;
		    p = VNs[VID]->edgenum+10;
		}
	    }
            SPath(can[i],neighbor[j],pathtemp);
            EEmbedTo(VID, k ,pathtemp);
	}

	temp = isFeasible(*bufSN);

	if( temp == 1){
	    embedArr[can[i]] = NID;
	    i = EPS+1;
	}
	else
	{    
	    SNcpy(bufSN,tempSN); //NID를 can[i]로 임베딩하는 것을 실패했으므로, NID를 임베딩 하기 전의 상태로 되돌림.
	}           
	
    }

    free(can);
    free(costTemp);
    free(neighbor);
    free(tempnbr);
    for( i=0;i<nodenum;i++){
	free(BFSArr[i]);
    }
    free(BFSArr);
    
    return temp;
}

//NEmbedTo : 노드를 임베딩 한다. 물리네트워크의 임베딩 되는 노드에 filledW 업데이트하고, 가상노드에 임베딩 정보 저장
void NEmbedTo(int VID, int from, int to){
    //노드 임베딩. Snet과 Vnet의 정보 변경작업 (bufSN)
    fprintf(f,"NEmbedTo %d->%d\n",from, to);
    bufSN->nodes[to]->filledW+=VNs[VID]->nodes[from]->weight;
    VNs[VID]->nodes[from]->embeddedSNID = to;
}

//EEmbedTo : 링크를 임베딩 한다. 위와 거의 비슷
void EEmbedTo(int VID, int from, int* to){
    //링크 임베딩. to는 path. (bufSN)

    int i;
    fprintf(f,"EEmbedTo %d -> Path : ", from);
    for( i=0;i<MAXPATH&&to[i]>=0;i++){
	fprintf(f," %d",to[i]);
	bufSN->edges[to[i]]->filledW +=VNs[VID]->edges[from]->weight;
	VNs[VID]->edges[from]->embeddedpath[i] = to[i];
   }
    fprintf(f,"\n");
}

//임베딩 해제. 요청시간 다되서 나가는 네트워크가 있을 때 정보 업데이트
void Unmap(int VID){
    //해당 Vnet 임베딩 해제(SN)
    int i,j,k;
    int nodenum = VNs[VID]->nodenum;
    int edgenum = VNs[VID]->edgenum;
     
    for( i=0;i<nodenum;i++){
	SN->nodes[VNs[VID]->nodes[i]->embeddedSNID]->filledW-=VNs[VID]->nodes[i]->weight;
	VNs[VID]->nodes[i]->embeddedSNID=-1;
    }
     
    for( i=0;i<edgenum;i++){
	for( j=0;j<MAXPATH;j++){
	    if( VNs[VID]->edges[i]->embeddedpath[j]>=0){
		SN->edges[VNs[VID]->edges[i]->embeddedpath[j]]->filledW-=VNs[VID]->edges[i]->weight;
		VNs[VID]->edges[i]->embeddedpath[j]=-1;
	    }
	}
    }
    VNs[VID]->isEmbedded=0;
}

int isFeasible(Snet FSN){
    //피져블 체크(bufSN), 피져블 : 1, 아니면 0
    int i, j;
    int nodenum = FSN.nodenum;
    int edgenum = FSN.edgenum;
    double k;
    double temp;
    double largest; // 링크 weight max값
    double* costTemp;

      
    if( CONFMODE == 1){//1 hop model
        costTemp = (double *)malloc(sizeof(double)*nodenum);
        largest = 0;
        for( i=0;i<nodenum;i++){
	    k=0;
	    for( j=0;j<FSN.nodes[i]->edgenum;j++){
		temp=(double)(FSN.edges[(FSN.nodes[i]->links[j])]->filledW)/(double)(FSN.edges[(FSN.nodes[i]->links[j])]->weight);
		k += temp;
		if(largest<temp)largest = temp;
	    }
	    costTemp[i]=k;
        }

        j=0;
        for( i=1;i<nodenum;i++){
	    if(costTemp[i]>=costTemp[j])j=i;
        }
        
        if( costTemp[j]<=(2/3) || (costTemp[j]+largest)<=1){
            
	    free(costTemp);
            return 1;
        }
        else{
	    free(costTemp);
	    return 0;
        }
    }
    else{// conflict graph 사용 
	costTemp = (double *)malloc(sizeof(double)*edgenum);
	for( i=0;i<edgenum;i++){
	    k = (double)(FSN.edges[i]->filledW)/(double)(FSN.edges[i]->weight);
	    for( j=0;j<edgenum;j++){
		if(conflictG[i][j]==1){
		    k += (double)(FSN.edges[j]->filledW)/(double)(FSN.edges[j]->weight);
		}
	    }
	    costTemp[i] = k;
	}

	j=0;
	for( i=1;i<edgenum;i++){
	    if(costTemp[i]>costTemp[j])j=i;
	}
         
	if( costTemp[j]<=1){
	      
	    free(costTemp);
	    return 1;
	}
	else{
	    free(costTemp);
	    return 0;
	}
             
    }
        
}

//물리네트워크 카피. 트라이용을 만들어 트라이해보고 피져블 하지 않으면 다시 원본복구, 피져블하면 원본에 트라이용을 카피하는 등의 용도
void SNcpy(Snet* to, Snet* from){

    int i;

    for( i=0;i<to->nodenum;i++){
	to->nodes[i]->filledW = from->nodes[i]->filledW;
    }

    for( i=0;i<to->edgenum;i++){
	to->edges[i]->filledW = from->edges[i]->filledW;
    }
}

//컨플릭트 그래프 만든다. CONFMODE값에 따라.
void mkConflictG(){
     //conflict graph
    int edgenum = SN->edgenum;
    int i, j, k;
    int x, y;
    int ** onehop;
     
        
    for( i=0;i<MAXSLINKNUM;i++){
	for( j=0;j<MAXSLINKNUM;j++){
	    conflictG[i][j]=0;
	}
    }
     
    if( CONFMODE == 2){//2hop
	onehop = (int**)malloc(sizeof(int*)*edgenum);
	for( i=0;i<edgenum;i++){
	    onehop[i] = (int*)malloc(sizeof(int)*edgenum);
	    for( j=0;j<edgenum;j++){
		onehop[i][j]=0;
	    }
	}
         
	for( i=0;i<edgenum;i++){
	    for( j=i+1;j<edgenum;j++)
	    {
		if((SN->edges[i]->node1==SN->edges[j]->node1)||(SN->edges[i]->node1==SN->edges[j]->node2)||(SN->edges[i]->node2==SN->edges[j]->node1)||(SN->edges[i]->node2==SN->edges[j]->node2))
		{
		    onehop[i][j] = 1;
		    onehop[j][i] = 1;
		    conflictG[i][j] = 1;
		    conflictG[j][i] = 1;
		}

	    }
	}
          
	    for( i=0;i<edgenum;i++){
		for( j=0;j<edgenum;j++){
		    if( onehop[i][j]==1){
			for( k=0;k<edgenum;k++){
			    if(onehop[j][k]==1)conflictG[i][k] = 1;
			}
		    }
		}
		conflictG[i][i] = 0;
	    }
	   
	    /*  fprintf(f,"Onehop Graph \n");
	    for( i=0;i<edgenum;i++){
		for( j=0;j<edgenum;j++){
		    fprintf(f,"%d ",onehop[i][j]);
		}
		fprintf(f,"\n");*/
    
	    for( i=0;i<edgenum;i++)free(onehop[i]);
	    free(onehop);
    }


    if( CONFMODE == 3){//distance model
	onehop = (int**)malloc(sizeof(int*)*2);
	onehop[0] = (int*)malloc(sizeof(int)*edgenum); //물리 링크의 중점의 x좌표들
	onehop[1] = (int*)malloc(sizeof(int)*edgenum); //물리 링크의 중점의 y좌표들
         
	for( i=0;i<edgenum;i++){
	    onehop[0][i] = ((SN->nodes[SN->edges[i]->node1]->x)+(SN->nodes[SN->edges[i]->node2]->x))/2;
	    onehop[1][i] = ((SN->nodes[SN->edges[i]->node1]->y)+(SN->nodes[SN->edges[i]->node2]->y))/2;
	}
	for( i=0;i<edgenum;i++){
	    x = onehop[0][i];
	    y = onehop[1][i];
	    for( j=0;j<edgenum;j++){
		if((((x - onehop[0][j])*(x - onehop[0][j]))+((y - onehop[1][j])*(y - onehop[1][j])))<=CONFLINKD*CONFLINKD)conflictG[i][j]=1;
                   
	    }
	}
	free(onehop[0]);
	free(onehop[1]);
	free(onehop);
    }
    
    else{//else. not emplemented.
          
    }
/*    fprintf(f,"Conflict Graph \n");
    for( i=0;i<edgenum;i++){
	for( j=0;j<edgenum;j++){
	    fprintf(f,"%d ",conflictG[i][j]);
	}
	fprintf(f,"\n");*/
}



//물리네트워크 생성하는 함수
void MakeSN(int nodenum, int xrange, int yrange, int cvmin, int cvmax, int nwmin, int nwmax, int lwmin, int lwmax){
    int nodecv[SNODENUM];		//node coverage radius
    int nodepo[SNODENUM][2];	//node position : -1 - not linked
    int i,j,k,p,q;
    double db;                                    // db 는 coverage radius의 합 - 거리
    int top, bot;
    int temp;
    int links;                                      //링크 개수 
    int* queue = (int*)malloc(sizeof(int)*nodenum);
    int* schd = (int*)malloc(sizeof(int)*nodenum);
    int linkwt[SNODENUM][SNODENUM];
    int cpu[SNODENUM];
	
    fprintf(f,"Start to Make SN\n");
    q=0;
    randomize(); //난수 발생기 시작점 초기화
    while(q==0){
	//initialize
	links=0;
	for(i=0;i<SNODENUM;i++){
	    for(j=0;j<SNODENUM;j++){
		linkwt[i][j]=0;
	    }
	    cpu[i]=0;
	    nodecv[i]=0;
	    nodepo[i][0]=-1;
	    nodepo[i][1]=-1;
	}
	
	// node position - random
		
	for(i=0;i<nodenum;i++){
	    for(j=0;j==0;){	//while 쓰기 귀찮아서 for. j=0은 노드 위치가 겹치는 경우 발생 -> 다시 루프 돌아서 새 위치 정한다.
		nodepo[i][0]=random(xrange);
		nodepo[i][1]=random(yrange);
		j=1;		///안겹치면 j=1인 상태로 루프 종료.
		for(k=0;k<i;k++){
		    if((nodepo[i][0]==nodepo[k][0])&&(nodepo[i][1]==nodepo[i][1]))j=0;	//노드 위치 겹치는 경우
		}
	    }
	}

	// node coverage, weight - random
	for(i=0;i<nodenum;i++){
	    cpu[i] = (random(nwmax-nwmin)+nwmin);
	    nodecv[i] = random(cvmax-cvmin)+cvmin;
	}
	
	// link
	links=0;
	for(i=0;i<nodenum;i++){
	    for(j=0;j<i;j++){
		p = ((nodepo[i][0]-nodepo[j][0])*(nodepo[i][0]-nodepo[j][0]))+((nodepo[i][1]-nodepo[j][1])*(nodepo[i][1]-nodepo[j][1]));
		db = nodecv[i] + nodecv[j] - sqrt((double)p);		// db 는 coverage radius의 합 - 거리
		if(db>0){										//연결 된 경우
		    linkwt[i][j]=random(lwmax-lwmin)+lwmin;		//link의 weight는 랜덤으로
		    linkwt[j][i]=random(lwmax-lwmin)+lwmin;		//link의 weight는 랜덤으로 : 반대도 일단 저장해 준다.
		    links++;
		}
	    }
	}
		
	//모든 노드가 링크로 연결되었는지 확인, 안되었으면 다시한다

	memset(queue,-1,nodenum*sizeof(int));
	memset(schd,0,nodenum*sizeof(int));

	schd[0]=1;
	queue[0]=0;
	top=1;
	bot=0;

	while(top!=bot){		//BFS
			
	    temp=queue[bot];	//dequeue
	    queue[bot]=-1;
	    bot=(bot+1)%nodenum;

			
	    for(j=0;j<nodenum;j++){	//neighbor들 enqueue및 서치목록 등록
		if(schd[j]==0&&linkwt[temp][j]>0){
		    schd[j]=1;
		    queue[top]=j;		//enqueue
		    top=(top+1)%nodenum;
		}
	    }
	}

	j=0;		//BFS 결과 서치되지 않은 노드가 있는지 확인
	for(i=0;i<nodenum;i++){
	    if(schd[i]==0){
		j=1;
	    }
	}
	if( j==0){ //다연결된 경우
	    q=1;
	    fprintf(f,"Succeed to Make Substrate Network -> Save in class SN\n");
	    SN = new Snet(nodenum, links);
	    bufSN = new Snet(nodenum, links);
	    tempSN = new Snet(nodenum, links);
	    k=0;
	    for( i=0;i<nodenum;i++){
                 
		for( j=i;j<nodenum;j++){
		    if( linkwt[i][j]>0){
			SN->edges[k] = new Sedge();
			bufSN->edges[k] = new Sedge();
			tempSN->edges[k] = new Sedge();
			SN->edges[k]->edgeID = k;
			SN->edges[k]->node1 = i;
			SN->edges[k]->node2 = j;
			SN->edges[k]->filledW = 0;
			SN->edges[k]->weight = linkwt[i][j];
			bufSN->edges[k]->edgeID = k;
			bufSN->edges[k]->node1 = i;
			bufSN->edges[k]->node2 = j;
			bufSN->edges[k]->filledW = 0;
			bufSN->edges[k]->weight = linkwt[i][j];
			tempSN->edges[k]->edgeID = k;
			tempSN->edges[k]->node1 = i;
			tempSN->edges[k]->node2 = j;
			tempSN->edges[k]->filledW = 0;
			tempSN->edges[k]->weight = linkwt[i][j];
			linkwt[i][j] = k+1; //용도 변경 : 노드 i,j를 잇는 링크번호 + 1, 밑에 쓰임.
			linkwt[j][i] = k+1;
			SN->map[i][j] = 1;
			SN->map[j][i] = 1;
			bufSN->map[i][j] = 1;
			bufSN->map[j][i] = 1;
			tempSN->map[i][j] = 1;
			tempSN->map[j][i] = 1;
			k++;
		    }
		}
	    }
	    for( i=0;i<nodenum;i++){
		k=0;
		for( j=0;j<nodenum;j++){
		    if(linkwt[i][j]>0)k++;
		}
		SN->nodes[i] = new Snode( k);
		bufSN->nodes[i] = new Snode( k);
		tempSN->nodes[i] = new Snode( k);
		SN->nodes[i]->x = nodepo[i][0];
		SN->nodes[i]->y = nodepo[i][1];
		SN->nodes[i]->weight = cpu[i];
		SN->nodes[i]->filledW = 0;
		SN->nodes[i]->nodeID = i;
		bufSN->nodes[i]->x = nodepo[i][0];
		bufSN->nodes[i]->y = nodepo[i][1];
		bufSN->nodes[i]->weight = cpu[i];
		bufSN->nodes[i]->filledW = 0;
		bufSN->nodes[i]->nodeID = i;
		tempSN->nodes[i]->x = nodepo[i][0];
		tempSN->nodes[i]->y = nodepo[i][1];
		tempSN->nodes[i]->weight = cpu[i];
		tempSN->nodes[i]->filledW = 0;
		tempSN->nodes[i]->nodeID = i;
		p=0;
		for( j=0;j<nodenum&&p<k;j++){
		    if( linkwt[i][j]>0){
			SN->nodes[i]->links[p] = linkwt[i][j]-1;
			bufSN->nodes[i]->links[p] = linkwt[i][j]-1;
			tempSN->nodes[i]->links[p] = linkwt[i][j]-1;
			p++;
		    }
		}
		SN->nodes[i]->edgenum = p;
		bufSN->nodes[i]->edgenum = p;
		tempSN->nodes[i]->edgenum = p;
	    }
	}
                 
	else{
	    q=0;
	    fprintf(f,"Made SN but Not connected -> Remake!\n");
	}
	fprintf(f,"SN Make End\n");
    }

    free(queue);
    free(schd);

    queue = NULL;
    schd = NULL;
     
}


//가상네트워크 생성하는 함수
void MakeVNs(int vnnum, int nwmin, int nwmax, int lwmin, int lwmax){ //Vnnum 만큼의 가상 네트워크 생성
    int i,j,k,p,q;
    int cpu[MAXVNODENUM];
    int nodes;
    int links;
    int linkwt[MAXVNODENUM][MAXVNODENUM];
    int top, bot;
    int temp;
    int queue[MAXVNODENUM];
    int schd[MAXVNODENUM];
    int VNnum;
    int sT, rT; //요청 들어온 시각, 요청의 지속 시간 
    int poi; //poisson 변수
    int jet;
    int count = 1;
        
    randomize();
    fprintf(f,"Start to MakeVN\n");
    VNs = (Vnet **)malloc(sizeof(Vnet*)*vnnum);
    
    q=0;
    poi = poisson(POIMEAN);
    while(poi==0)
    {
	q++;
	poi=poisson(POIMEAN);
    }

    fprintf(f, "poi first = %d \n",poi);

    for(VNnum=0;VNnum<vnnum;VNnum++){
	jet=1;
	while(jet==1){
	    for( i=0; i<MAXVNODENUM;i++){
		cpu[i] = 0;
		for( j=0;j<MAXVNODENUM;j++){
		    linkwt[i][j] = 0;
		}
	    }
	    nodes = 0;
	    top=0;
	    bot=0;
	    sT=0;
	    rT=0;
	    temp=0;
	    links = 0;
          
	    nodes = random(MAXVNODENUM-3)+3; // 가상 네트워크 노드 개수 설정

	    for( i=0;i<MAXVNODENUM;i++){
		schd[i] = 0;
		queue[i] = 0;
	    }


	    for(i =0; i<nodes; i++)cpu[i] = (random(nwmax-nwmin)+nwmin); 
   
	    links = 0; 
    
	    for( i =0; i<nodes; i++){                                                            
		for ( j =i+1; j<nodes; j++){
		    k = random(9);
		    if( k<4){    // 노드 i와 노드 j를 40프로의 확률로 잇는다. 
			linkwt[i][j] = random(lwmax-lwmin)+lwmin;
			links++;
		    }
		}
	    }
	    memset(queue,-1,nodes*sizeof(int));
	    memset(schd,0,nodes*sizeof(int));

	    schd[0]=1;
	    queue[0]=0;
	    top=1;
	    bot=0;

	    while( top!=bot){		//BFS
			     
		temp=queue[bot];	//dequeue
		queue[bot]=-1;
		bot=(bot+1)%nodes;

			
		for( j=0;j<nodes;j++){
		    if( schd[j]==0&&linkwt[temp][j]>0){
			schd[j]=1;
			queue[top]=j;		//enqueue
			top=(top+1)%nodes;
		    }
		}
	    }

	    jet=0;		
	    for( i=0;i<nodes;i++){
		if( schd[i]==0){
		    jet=1;
		}
	    }
          
	    if( jet==0){
	
		sT = random(TWIN)+q*TWIN;
		rT = (exponential(EXPMEAN-1)+1)*TWIN;
	   
		fprintf(f,"Succeed to Make Virtual network %d : %d %d %d %d\n", VNnum, nodes, links, sT, rT);
		VNs[VNnum] = new Vnet( nodes, links, sT, rT);
		k=0;
		for( i=0;i<nodes;i++){
                 
		    for( j=i;j<nodes;j++){
			if( linkwt[i][j]>0){
			    VNs[VNnum]->edges[k] = new Vedge();
			    VNs[VNnum]->edges[k]->edgeID = k;
			    VNs[VNnum]->edges[k]->node1 = i;
			    VNs[VNnum]->edges[k]->node2 = j;
			    for( p=0;p<MAXPATH;p++)VNs[VNnum]->edges[k]->embeddedpath[p] = -1;
			    VNs[VNnum]->edges[k]->weight = linkwt[i][j];
			    linkwt[i][j] = k+1;
			    linkwt[j][i] = k+1;
			    VNs[VNnum]->map[i][j]=1;
			    VNs[VNnum]->map[j][i]=1;
			    k++;
			}
		    }
		}
		for( i=0;i<nodes;i++){
		    k=0;
		    for( j=0;j<nodes;j++){
			if(linkwt[i][j]>0)k++;
		    }
		    VNs[VNnum]->nodes[i] = new Vnode( k);
		    VNs[VNnum]->nodes[i]->weight = cpu[i];
		    VNs[VNnum]->nodes[i]->embeddedSNID = -1;
		    p=0;
		    for( j=0;j<nodes&&p<k;j++){
			if( linkwt[i][j]>0){
			    VNs[VNnum]->nodes[i]->links[p] = linkwt[i][j]-1;
			    p++;
			}
		    }
		    VNs[VNnum]->nodes[i]->edgenum = p;
		}

		if(count==poi) //한 타임 윈도우에 들어온 가상 네트워크의 개수가 poission변수와 같으면..
		{
		    count=1;
		    poi = poisson(POIMEAN);
		    while(poi==0)
		    {
			poi=poisson(POIMEAN);
			q++;
		    }
		    q++;
		    fprintf(f, "poi = %d \n",poi);
		}
		else
		    count++;
	    }

	}
    }
    fprintf(f,"VNs Make End\n");

}

//poisson 분포를 따르는 변수 하나 생성
int poisson(int poimean){
 
    int poi = 0;
    double u, p, f;
      
    f = p = exp(-1*(double)poimean); 
    u = (double)rand()/RAND_MAX;
	
    while (f <= u)
    {
	p *= ((double)poimean/(poi+1.0));
	f += p;
	poi++;
    }
    
    return poi;
}

//exponential 분포를 따르는 변수 하나 생성
int exponential(int expmean){
 
    int exp;
    double u;
        
    u = (double)rand()/RAND_MAX;
    exp = (int)-expmean*log(1-u); 
    
    return exp;
}

//가상 네트워크 VN의 Revenue 계산.
int VNRevenue(Vnet* VN){
    
    int i;
    int revenue=0;
    
    for(i=0;i<VN->nodenum;i++)
	revenue += VN->nodes[i]->weight;
    
    for(i=0;i<VN->edgenum;i++)
	revenue += ALPHA*(VN->edges[i]->weight);

    return revenue ;
}
    
void VNscpy(Vnet** to, Vnet** from){
    
    int i;
    for(i=0;i<VNNUM;i++)
	memcpy(to[i],from[i],sizeof(Vnet));    

}    
    
int CompEmbed(int VID, int* embedArr){ //VID번째 가상 네트워크를 임베딩한다.
    
    int i,j,k,r;
    int mapok;
    int* can;
    int count;
    int vnode1, vnode2,snode1, snode2;
    int pathtemp[MAXPATH];

    SNcpy(bufSN,SN);

    can = (int*)malloc(sizeof(int)*bufSN->nodenum);

    //노드 임베딩 
    for(i=0;i<VNs[VID]->nodenum;i++){//모든 노드에 대한 임베딩 
	fprintf(f1,"Node Embed VN: %d, Nodenum %d \n",VID,i);
	
	count =0;
	mapok = 0;
	for(j=0;j<bufSN->nodenum;j++){ //가상 노드 i를 임베딩 할 수 있는 물리  노드가 있는지 체크
	    if((embedArr[j]==-1)&&(VNs[VID]->nodes[i]->weight<= (bufSN->nodes[j]->weight-bufSN->nodes[j]->filledW))){
		mapok=1;
		can[count]=j;
		count++;
		}
	}
	
	if(mapok==0)
	{
	    SNcpy(bufSN,SN); //임베딩 중간에 노드 임베딩에 실패할 경우 bufSN을 VID 가상 네트워크 임베딩 하기 전으로 돌려 놓는다.
	    free(can);
	    return 0;	    
	}  

	//가상 노드 i보다 자원많은 물리노드 들이 있으므로... 그것들을 random하게 찾는다.
	while(1){
	    mapok = 1;
	    r = can[random(count)];
	    for(k=0;k<VNs[VID]->nodenum;k++){
		if((VNs[VID]->nodes[k]->embeddedSNID==r)||(VNs[VID]->nodes[i]->weight > (bufSN->nodes[r]->weight-bufSN->nodes[r]->filledW))){
		    mapok=0;
		}
	    }
	    if(mapok==1)
	    {
		bufSN->nodes[r]->filledW+=VNs[VID]->nodes[i]->weight;
		VNs[VID]->nodes[i]->embeddedSNID = r;
		embedArr[r]=i;
		break;
	    }
	}
    } 
    
    //링크 임베딩 
    for(i=0;i<VNs[VID]->edgenum;i++)
    {
	vnode1 = VNs[VID]->edges[i]->node1;
	vnode2 = VNs[VID]->edges[i]->node2;
	
	snode1 = VNs[VID]->nodes[vnode1]->embeddedSNID;
	snode2 = VNs[VID]->nodes[vnode2]->embeddedSNID;
	
	SPath(snode1,snode2,pathtemp);

	fprintf(f1,"EEmbedTO %d -> Path : ",i);
	
	for(j=0;j< MAXPATH && pathtemp[j]>=0;j++) {
		fprintf(f1,"%d",pathtemp[j]);
		bufSN->edges[pathtemp[j]]->filledW += VNs[VID]->edges[i]->weight;
		VNs[VID]->edges[i]->embeddedpath[j]=pathtemp[j];  
	}
	fprintf(f1,"\n");	    
    }
	
    mapok = isFeasible(*bufSN);
	
    if(mapok) //위의 노드 및 링크 임베딩이 feasible할 경우 
    {
	SNcpy(SN,bufSN);
    }
    else
	SNcpy(bufSN,SN);
    
    free(can);
    return mapok;
}

