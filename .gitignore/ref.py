import sys
from copy import deepcopy

WP,WN,WB,WR,WQ,WK = 'P','N','B','R','Q','K'
BP,BN,BB,BR,BQ,BK = 'p','n','b','r','q','k'

def is_white(p): return p is not None and p.isupper()
def is_black(p): return p is not None and p.islower()

class Board:
    def __init__(self, fen):
        self.load(fen)

    def load(self, fen):
        parts = fen.split()
        rows = parts[0].split('/')
        self.b = [[None]*8 for _ in range(8)]  # b[rank][file], rank0=1st rank
        for i, row in enumerate(rows):
            rank = 7 - i
            file = 0
            for c in row:
                if c.isdigit():
                    file += int(c)
                else:
                    self.b[rank][file] = c
                    file += 1
        self.white_to_move = parts[1] == 'w'
        self.castle = set(parts[2]) if parts[2] != '-' else set()
        if parts[3] != '-':
            f = ord(parts[3][0]) - ord('a')
            r = int(parts[3][1]) - 1
            self.ep = (r, f)
        else:
            self.ep = None

    def piece_at(self, r, f):
        if 0 <= r < 8 and 0 <= f < 8:
            return self.b[r][f]
        return None

    def find_king(self, white):
        target = 'K' if white else 'k'
        for r in range(8):
            for f in range(8):
                if self.b[r][f] == target:
                    return (r, f)
        return None

    def attacked_by(self, r, f, by_white):
        # pawn
        if by_white:
            for df in (-1, 1):
                pr, pf = r-1, f+df
                if 0<=pr<8 and 0<=pf<8 and self.b[pr][pf] == 'P':
                    return True
        else:
            for df in (-1, 1):
                pr, pf = r+1, f+df
                if 0<=pr<8 and 0<=pf<8 and self.b[pr][pf] == 'p':
                    return True
        # knight
        for dr, df in [(1,2),(2,1),(-1,2),(-2,1),(1,-2),(2,-1),(-1,-2),(-2,-1)]:
            pr, pf = r+dr, f+df
            if 0<=pr<8 and 0<=pf<8:
                p = self.b[pr][pf]
                if p and p.upper()=='N' and (p.isupper()==by_white):
                    return True
        # king
        for dr in (-1,0,1):
            for df in (-1,0,1):
                if dr==0 and df==0: continue
                pr, pf = r+dr, f+df
                if 0<=pr<8 and 0<=pf<8:
                    p = self.b[pr][pf]
                    if p and p.upper()=='K' and (p.isupper()==by_white):
                        return True
        # sliders: rook/queen
        for dr, df in [(1,0),(-1,0),(0,1),(0,-1)]:
            pr, pf = r+dr, f+df
            while 0<=pr<8 and 0<=pf<8:
                p = self.b[pr][pf]
                if p:
                    if p.isupper()==by_white and p.upper() in ('R','Q'):
                        return True
                    break
                pr += dr; pf += df
        # bishop/queen
        for dr, df in [(1,1),(1,-1),(-1,1),(-1,-1)]:
            pr, pf = r+dr, f+df
            while 0<=pr<8 and 0<=pf<8:
                p = self.b[pr][pf]
                if p:
                    if p.isupper()==by_white and p.upper() in ('B','Q'):
                        return True
                    break
                pr += dr; pf += df
        return False

    def in_check(self, white):
        kr, kf = self.find_king(white)
        return self.attacked_by(kr, kf, not white)

    def gen_pseudo(self):
        moves = []
        w = self.white_to_move
        for r in range(8):
            for f in range(8):
                p = self.b[r][f]
                if not p: continue
                if p.isupper() != w: continue
                t = p.upper()
                if t == 'P':
                    self._pawn(r,f,w,moves)
                elif t == 'N':
                    for dr, df in [(1,2),(2,1),(-1,2),(-2,1),(1,-2),(2,-1),(-1,-2),(-2,-1)]:
                        nr, nf = r+dr, f+df
                        if 0<=nr<8 and 0<=nf<8:
                            q = self.b[nr][nf]
                            if q is None or q.isupper()!=w:
                                moves.append((r,f,nr,nf,None,'N'))
                elif t == 'K':
                    for dr in (-1,0,1):
                        for df in (-1,0,1):
                            if dr==0 and df==0: continue
                            nr, nf = r+dr, f+df
                            if 0<=nr<8 and 0<=nf<8:
                                q = self.b[nr][nf]
                                if q is None or q.isupper()!=w:
                                    moves.append((r,f,nr,nf,None,'K'))
                    self._castle(r,f,w,moves)
                else:
                    dirs = []
                    if t in ('R','Q'): dirs += [(1,0),(-1,0),(0,1),(0,-1)]
                    if t in ('B','Q'): dirs += [(1,1),(1,-1),(-1,1),(-1,-1)]
                    for dr, df in dirs:
                        nr, nf = r+dr, f+df
                        while 0<=nr<8 and 0<=nf<8:
                            q = self.b[nr][nf]
                            if q is None:
                                moves.append((r,f,nr,nf,None,t))
                            else:
                                if q.isupper()!=w:
                                    moves.append((r,f,nr,nf,None,t))
                                break
                            nr += dr; nf += df
        return moves

    def _pawn(self, r, f, w, moves):
        d = 1 if w else -1
        start = 1 if w else 6
        promo_rank = 7 if w else 0
        nr = r+d
        if 0<=nr<8 and self.b[nr][f] is None:
            if nr == promo_rank:
                for pr in ('Q','R','B','N'):
                    moves.append((r,f,nr,f,pr,'P'))
            else:
                moves.append((r,f,nr,f,None,'P'))
            if r == start and self.b[r+2*d][f] is None:
                moves.append((r,f,r+2*d,f,None,'P'))
        for df in (-1,1):
            nf = f+df
            if 0<=nf<8 and 0<=nr<8:
                q = self.b[nr][nf]
                if q and q.isupper()!=w:
                    if nr == promo_rank:
                        for pr in ('Q','R','B','N'):
                            moves.append((r,f,nr,nf,pr,'P'))
                    else:
                        moves.append((r,f,nr,nf,None,'P'))
                elif self.ep == (nr, nf):
                    moves.append((r,f,nr,nf,'EP','P'))

    def _castle(self, r, f, w, moves):
        if w and r==0 and f==4:
            if 'K' in self.castle and self.b[0][5] is None and self.b[0][6] is None and self.b[0][7]=='R':
                if not self.attacked_by(0,4,False) and not self.attacked_by(0,5,False) and not self.attacked_by(0,6,False):
                    moves.append((0,4,0,6,None,'CASTLE'))
            if 'Q' in self.castle and self.b[0][3] is None and self.b[0][2] is None and self.b[0][1] is None and self.b[0][0]=='R':
                if not self.attacked_by(0,4,False) and not self.attacked_by(0,3,False) and not self.attacked_by(0,2,False):
                    moves.append((0,4,0,2,None,'CASTLE'))
        if (not w) and r==7 and f==4:
            if 'k' in self.castle and self.b[7][5] is None and self.b[7][6] is None and self.b[7][7]=='r':
                if not self.attacked_by(7,4,True) and not self.attacked_by(7,5,True) and not self.attacked_by(7,6,True):
                    moves.append((7,4,7,6,None,'CASTLE'))
            if 'q' in self.castle and self.b[7][3] is None and self.b[7][2] is None and self.b[7][1] is None and self.b[7][0]=='r':
                if not self.attacked_by(7,4,True) and not self.attacked_by(7,3,True) and not self.attacked_by(7,2,True):
                    moves.append((7,4,7,2,None,'CASTLE'))

    def make(self, m):
        r,f,nr,nf,promo,ptype = m
        w = self.white_to_move
        state = (deepcopy(self.b), self.white_to_move, set(self.castle), self.ep)
        p = self.b[r][f]
        self.ep_new = None
        if ptype == 'P' and abs(nr-r)==2:
            self.ep_new = ((r+nr)//2, f)
        # en passant capture
        if promo == 'EP':
            self.b[r][nf] = None  # captured pawn is on same rank as mover, target file
        self.b[nr][nf] = p
        self.b[r][f] = None
        if promo not in (None, 'EP'):
            self.b[nr][nf] = promo if w else promo.lower()
        if ptype == 'CASTLE':
            if nf == 6:
                self.b[r][5] = self.b[r][7]; self.b[r][7] = None
            else:
                self.b[r][3] = self.b[r][0]; self.b[r][0] = None
        # update castling rights
        def remove(c):
            self.castle.discard(c)
        if p == 'K': remove('K'); remove('Q')
        if p == 'k': remove('k'); remove('q')
        if (r,f) == (0,0) or (nr,nf)==(0,0): remove('Q')
        if (r,f) == (0,7) or (nr,nf)==(0,7): remove('K')
        if (r,f) == (7,0) or (nr,nf)==(7,0): remove('q')
        if (r,f) == (7,7) or (nr,nf)==(7,7): remove('k')
        self.ep = self.ep_new
        self.white_to_move = not self.white_to_move
        return state

    def unmake(self, state):
        self.b, self.white_to_move, self.castle, self.ep = state

def perft(bd, depth):
    if depth == 0:
        return 1
    total = 0
    moves = bd.gen_pseudo()
    mover_white = bd.white_to_move
    for m in moves:
        state = bd.make(m)
        if not bd.in_check(mover_white):
            total += perft(bd, depth-1)
        bd.unmake(state)
    return total

def sq_name(r,f):
    return chr(ord('a')+f) + str(r+1)

def divide(bd, depth):
    moves = bd.gen_pseudo()
    mover_white = bd.white_to_move
    total = 0
    results = []
    for m in moves:
        r,f,nr,nf,promo,ptype = m
        state = bd.make(m)
        if not bd.in_check(mover_white):
            n = perft(bd, depth-1)
            promo_c = {'Q':'q','R':'r','B':'b','N':'n'}.get(promo,'') if promo not in (None,'EP') else ''
            name = sq_name(r,f)+sq_name(nr,nf)+promo_c
            results.append((name, n))
            total += n
        bd.unmake(state)
    results.sort()
    for name, n in results:
        print(f"{name} : {n}")
    print(f"Total = {total}")

if __name__ == '__main__':
    fen = sys.argv[1]
    depth = int(sys.argv[2])
    bd = Board(fen)
    divide(bd, depth)