#pragma once
#include <stdint.h>
#include <string.h>

// Planejador de grade totalmente embarcado, independente de Arduino.
// N=0/E=1/S=2/O=3; bit de parede = 1<<direcao.
namespace mazeplan {
enum Direction : uint8_t { NORTH=0, EAST=1, SOUTH=2, WEST=3 };
struct Cell { uint8_t known=0, wall=0, visited=0, traversed=0; };
struct Next { bool valid=false; Direction dir=NORTH; uint8_t x=0,y=0; };
class Grid {
public:
    static constexpr uint8_t MAX=16;
    uint8_t width=4, height=4, goalX=1, goalY=1;
    Cell cell[MAX][MAX];
    bool init(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy) {
        if(w<2 || h<2 || w>MAX || h>MAX || gx>=w || gy>=h)return false;
        width=w;height=h;goalX=gx;goalY=gy;
        memset(cell,0,sizeof(cell));
        for(uint8_t y=0;y<h;y++)for(uint8_t x=0;x<w;x++) {
            if(y==h-1) setBoundary(x,y,NORTH);
            if(x==w-1) setBoundary(x,y,EAST);
            if(y==0) setBoundary(x,y,SOUTH);
            if(x==0) setBoundary(x,y,WEST);
        }
        return true;
    }
    bool inside(int x,int y)const{return x>=0 && y>=0 && x<width && y<height;}
    static uint8_t wallBit(Direction d){return (uint8_t)(1u << (uint8_t)d);}
    static Direction opposite(Direction d){return (Direction)(((uint8_t)d+2)&3);}
    static Direction left(Direction d){return (Direction)(((uint8_t)d+3)&3);}
    static Direction right(Direction d){return (Direction)(((uint8_t)d+1)&3);}
    static int dx(Direction d){return d==EAST?1:d==WEST?-1:0;}
    static int dy(Direction d){return d==NORTH?1:d==SOUTH?-1:0;}
    bool observe(uint8_t x,uint8_t y,Direction d,bool wall) {
        if(!inside(x,y))return false;
        const int nx=x+dx(d),ny=y+dy(d);
        if(!inside(nx,ny) && !wall)return false;
        const uint8_t b=wallBit(d);
        Cell& a=cell[y][x];
        if((a.known&b) && ((a.wall&b)!=0)!=wall)return false; // inconsistencia, trava
        if(inside(nx,ny)) {
            Cell& n=cell[ny][nx];const uint8_t rev=wallBit(opposite(d));
            if((n.known&rev) && ((n.wall&rev)!=0)!=wall)return false;
        }
        a.known|=b; if(wall)a.wall|=b;else a.wall&=~b;
        if(inside(nx,ny)){
            Cell& n=cell[ny][nx];const uint8_t rev=wallBit(opposite(d));
            n.known|=rev;if(wall)n.wall|=rev;else n.wall&=~rev;
        }
        return true;
    }
    // V4.9.1: versao TOLERANTE usada pela navegacao real. Em vez de travar a
    // missao quando uma leitura discorda do mapa, a observacao mais recente
    // prevalece, com duas excecoes:
    //  - borda externa do labirinto continua parede (leitura "aberta" ignorada);
    //  - passagem ja PERCORRIDA fisicamente nao vira parede por leitura ruim.
    // Retorna false apenas para coordenada invalida.
    bool update(uint8_t x,uint8_t y,Direction d,bool wall) {
        if(!inside(x,y))return false;
        const int nx=x+dx(d),ny=y+dy(d);
        if(!inside(nx,ny))return true; // borda: ja e parede conhecida
        const uint8_t b=wallBit(d),rev=wallBit(opposite(d));
        if(wall && (cell[y][x].traversed&b))return true;
        Cell& a=cell[y][x];Cell& n=cell[ny][nx];
        a.known|=b;n.known|=rev;
        if(wall){a.wall|=b;n.wall|=rev;}else{a.wall&=~b;n.wall&=~rev;}
        return true;
    }
    bool isKnown(uint8_t x,uint8_t y,Direction d)const{return inside(x,y)&&(cell[y][x].known&wallBit(d));}
    bool open(uint8_t x,uint8_t y,Direction d)const {
        if(!inside(x,y))return false;
        const uint8_t b=wallBit(d);
        if(!(cell[y][x].known&b) || (cell[y][x].wall&b))return false;
        int nx=x+dx(d), ny=y+dy(d);
        return inside(nx,ny) && (cell[ny][nx].known&wallBit(opposite(d))) && !(cell[ny][nx].wall&wallBit(opposite(d)));
    }
    bool markTraversed(uint8_t x,uint8_t y,Direction d) {
        if(!open(x,y,d))return false;
        const int nx=x+dx(d),ny=y+dy(d);
        cell[y][x].traversed|=wallBit(d);
        cell[ny][nx].traversed|=wallBit(opposite(d));
        return true;
    }
    bool traversed(uint8_t x,uint8_t y,Direction d)const {
        return open(x,y,d) && (cell[y][x].traversed&wallBit(d)) &&
          (cell[y+dy(d)][x+dx(d)].traversed&wallBit(opposite(d)));
    }
    bool atGoal(uint8_t x,uint8_t y)const{return x==goalX && y==goalY;}
    // Busca em largura apenas por arestas COMPROVADAMENTE abertas.
    // Prioriza objetivo conhecido alcancavel; depois proxima celula nao visitada.
    // Retorno false significa nenhuma passagem CONFIRMADA, nunca autoriza atravessar desconhecida.
    Next next(uint8_t x,uint8_t y,Direction heading) const {
        Next bad;
        if(!inside(x,y) || atGoal(x,y))return bad;
        uint8_t qx[MAX*MAX], qy[MAX*MAX], previous[MAX][MAX];
        uint8_t firstDir[MAX][MAX];
        uint16_t distance[MAX][MAX]{};
        memset(previous,0,sizeof(previous));
        uint16_t start=0,end=0;
        qx[end]=x;qy[end++]=y;previous[y][x]=1;
        bool candidate=false;uint8_t cx=0,cy=0;
        uint16_t bestScore=65535,bestH=65535;
        // Pequena preferencia local: seguir, direita, esquerda, retornar.
        const Direction preference[4]={heading,right(heading),left(heading),opposite(heading)};
        while(start<end){
            const uint8_t px=qx[start],py=qy[start];++start;
            if(px==goalX && py==goalY){cx=px;cy=py;candidate=true;break;}
            if(!(px==x && py==y) && !cell[py][px].visited){
                const uint16_t h=(px>goalX?px-goalX:goalX-px)+(py>goalY?py-goalY:goalY-py);
                const uint16_t score=distance[py][px]+h;
                if(!candidate || score<bestScore || (score==bestScore && h<bestH)){
                    cx=px;cy=py;candidate=true;bestScore=score;bestH=h;
                }
            }
            for(uint8_t i=0;i<4;i++){
                Direction d=preference[i];if(!open(px,py,d))continue;
                const int nx=px+dx(d),ny=py+dy(d);
                if(previous[ny][nx])continue;
                previous[ny][nx]=1;
                distance[ny][nx]=distance[py][px]+1;
                firstDir[ny][nx]=(px==x && py==y)?(uint8_t)d:firstDir[py][px];
                qx[end]=(uint8_t)nx;qy[end]=(uint8_t)ny;++end;
            }
        }
        if(!candidate)return bad;
        const Direction chosen=(Direction)firstDir[cy][cx];
        return {true,chosen,(uint8_t)(x+dx(chosen)),(uint8_t)(y+dy(chosen))};
    }
private:
    void setBoundary(uint8_t x,uint8_t y,Direction d){cell[y][x].known|=wallBit(d);cell[y][x].wall|=wallBit(d);}
};
}
